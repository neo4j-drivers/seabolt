/*
 * Copyright (c) 2002-2019 "Neo4j,"
 * Neo4j Sweden AB [http://neo4j.com]
 *
 * This file is part of Neo4j.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "communication-plain.h"
#include "bolt-private.h"
#include "config-private.h"
#include "mem.h"
#include "name.h"
#include "status-private.h"
#include "log-private.h"

#define MAX_IPADDR_LEN 64
#define ADDR_SIZE(address) address->ss_family == AF_INET ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6)
#define TRY_SOCKET(code, status, error_ctx_fmt, file, line) { \
    int status_try = (code); \
    if (status_try == -1) { \
        int last_error = socket_last_error(); \
        int last_error_transformed = socket_transform_error(comm, last_error); \
        BoltStatus_set_error_with_ctx(status, last_error_transformed, error_ctx_fmt, file, line, last_error); \
        return BOLT_STATUS_SET; \
    } \
}

int socket_last_error();

int socket_transform_error(BoltCommunication* comm, int error_code);

int socket_ignore_sigpipe(struct sigaction* replaced_action);

int socket_restore_sigpipe(struct sigaction* action_to_restore);

int socket_lifecycle_startup();

int socket_lifecycle_shutdown();

int socket_open(int domain, int type, int protocol);

int socket_shutdown(int sockfd);

int socket_close(int sockfd);

int socket_connect(int sockfd, const struct sockaddr* addr, socklen_t addrlen, int* in_progress);

int socket_select(int sockfd, int timeout);

ssize_t socket_send(int sockfd, const void* buf, size_t len, int flags);

ssize_t socket_recv(int sockfd, void* buf, size_t len, int flags);

int socket_get_local_addr(int sockfd, struct sockaddr_storage* address, socklen_t* address_size);

int socket_disable_sigpipe(int sockfd);

int socket_set_blocking_mode(int sockfd, int blocking);

int socket_set_recv_timeout(int sockfd, int timeout);

int socket_set_keepalive(int sockfd, int keepalive);

int socket_set_nodelay(int sockfd, int nodelay);

int plain_socket_ignore_sigpipe(BoltCommunication* comm)
{
    PlainCommunicationContext* ctx = comm->context;
    int status = socket_ignore_sigpipe(&ctx->action_to_restore);
    if (status<0) {
        int last_error = socket_last_error();
        BoltStatus_set_error_with_ctx(comm->status, socket_transform_error(comm, last_error),
                "plain_socket_ignore_sigpipe(%s:%d): unable to install ignore handler for SIGPIPE: %d", __FILE__,
                __LINE__, last_error);
        return BOLT_STATUS_SET;
    }
    return BOLT_SUCCESS;
}

int plain_socket_restore_sigpipe(BoltCommunication* comm)
{
    PlainCommunicationContext* ctx = comm->context;
    int status = socket_restore_sigpipe(&ctx->action_to_restore);
    if (status<0) {
        int last_error = socket_last_error();
        BoltStatus_set_error_with_ctx(comm->status, socket_transform_error(comm, last_error),
                "plain_socket_ignore_sigpipe(%s:%d): unable to restore original handler for SIGPIPE: %d", __FILE__,
                __LINE__, last_error);
        return BOLT_STATUS_SET;
    }
    return BOLT_SUCCESS;
}

int plain_socket_open(BoltCommunication* comm, const struct sockaddr_storage* address)
{
    PlainCommunicationContext* context = comm->context;
    context->fd_socket = socket_open(address->ss_family, SOCK_STREAM, IPPROTO_TCP);
    if (context->fd_socket==-1) {
        int last_error_code = socket_last_error();
        BoltStatus_set_error_with_ctx(comm->status, socket_transform_error(comm, last_error_code),
                "plain_socket_open(%s,%d): socket error code: %d", __FILE__, __LINE__, last_error_code);
        return comm->status->error;
    }

    TRY_SOCKET(socket_disable_sigpipe(context->fd_socket), comm->status,
            "plain_socket_open(%s:%d), unable to set SO_NOSIGPIPE: %d", __FILE__, __LINE__);

    if (comm->sock_opts!=NULL && comm->sock_opts->connect_timeout>0) {
        // enable non-blocking mode
        TRY_SOCKET(socket_set_blocking_mode(context->fd_socket, 0), comm->status,
                "plain_socket_open(%s:%d), plain_socket_set_blocking error code: %d", __FILE__, __LINE__);

        // initiate connection
        int in_progress = 0;
        int conn_status = socket_connect(context->fd_socket, (struct sockaddr*) (address), ADDR_SIZE(address),
                &in_progress);
        if (conn_status==-1 && !in_progress) {
            int error_code = socket_last_error();
            BoltStatus_set_error_with_ctx(comm->status, socket_transform_error(comm, error_code),
                    "plain_socket_open(%s:%d), connect error code: %d", __FILE__, __LINE__, error_code);
            return BOLT_STATUS_SET;
        }

        if (conn_status) {
            conn_status = socket_select(context->fd_socket, comm->sock_opts->connect_timeout);
            switch (conn_status) {
            case 0: {
                //timeout expired
                BoltStatus_set_error_with_ctx(comm->status, BOLT_TIMED_OUT, "plain_socket_open(%s:%d)", __FILE__,
                        __LINE__);
                return BOLT_STATUS_SET;
            }
            case 1: {
                //connection successful
                break;
            }
            default: {
                int last_error = socket_last_error();
                BoltStatus_set_error_with_ctx(comm->status, socket_transform_error(comm, last_error),
                        "plain_socket_open(%s:%d), select error code: %d", __FILE__, __LINE__, last_error);
                return BOLT_STATUS_SET;
            }
            }
        }

        // revert to blocking mode
        TRY_SOCKET(socket_set_blocking_mode(context->fd_socket, 1), comm->status,
                "plain_socket_open(%s:%d), plain_socket_set_blocking error code: %d", __FILE__, __LINE__);
    }
    else {
        int in_progress = 0;
        TRY_SOCKET(socket_connect(context->fd_socket, (struct sockaddr*) (address), ADDR_SIZE(address), &in_progress),
                comm->status, "plain_socket_open(%s:%d), connect error code: %d", __FILE__, __LINE__);
    }

    char resolved_host[MAX_IPADDR_LEN], resolved_port[6];
    int status = get_address_components(address, resolved_host, MAX_IPADDR_LEN, resolved_port, 6);
    if (status!=0) {
        BoltStatus_set_error_with_ctx(comm->status, BOLT_ADDRESS_NAME_INFO_FAILED,
                "plain_socket_open(%s:%d), remote get_address_components error code: %d", __FILE__, __LINE__, status);
        return BOLT_STATUS_SET;
    }
    context->remote_endpoint = BoltAddress_create(resolved_host, resolved_port);

    char local_host[MAX_IPADDR_LEN], local_port[6];
    struct sockaddr_storage local_addr;
    socklen_t local_addr_size = address->ss_family==AF_INET ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6);
    TRY_SOCKET(socket_get_local_addr(context->fd_socket, &local_addr, &local_addr_size), comm->status,
            "plain_socket_open(%s:%d): getsockname error code: %d", __FILE__, __LINE__);

    status = get_address_components(&local_addr, local_host, MAX_IPADDR_LEN, local_port, 6);
    if (status!=0) {
        BoltStatus_set_error_with_ctx(comm->status, BOLT_ADDRESS_NAME_INFO_FAILED,
                "plain_socket_open(%s:%d), local get_address_components error code: %d", __FILE__, __LINE__, status);
        return BOLT_STATUS_SET;
    }
    context->local_endpoint = BoltAddress_create(local_host, local_port);

    TRY_SOCKET(socket_set_nodelay(context->fd_socket, 1), comm->status,
            "plain_socket_open(%s:%d), socket_set_nodelay error code: %d", __FILE__, __LINE__);
    TRY_SOCKET(socket_set_recv_timeout(context->fd_socket, comm->sock_opts->recv_timeout), comm->status,
            "plain_socket_open(%s:%d), socket_set_recv_timeout error code: %d", __FILE__, __LINE__);
    TRY_SOCKET(socket_set_keepalive(context->fd_socket, comm->sock_opts->keep_alive), comm->status,
            "plain_socket_open(%s:%d), socket_set_keepalive error code: %d", __FILE__, __LINE__);

    return BOLT_SUCCESS;
}

int plain_socket_close(BoltCommunication* comm)
{
    PlainCommunicationContext* context = comm->context;

    if (context->fd_socket) {
        socket_shutdown(context->fd_socket);
        socket_close(context->fd_socket);
        context->fd_socket = 0;
    }

    if (context->local_endpoint!=NULL) {
        BoltAddress_destroy(context->local_endpoint);
        context->local_endpoint = NULL;
    }

    if (context->remote_endpoint!=NULL) {
        BoltAddress_destroy(context->remote_endpoint);
        context->remote_endpoint = NULL;
    }

    return BOLT_SUCCESS;
}

int plain_socket_send(BoltCommunication* comm, char* buffer, int length, int* sent)
{
    PlainCommunicationContext* context = comm->context;

    ssize_t bytes = socket_send(context->fd_socket, buffer, (size_t) length, 0);
    if (bytes==-1) {
        int last_error = socket_last_error();
        BoltStatus_set_error_with_ctx(comm->status, socket_transform_error(comm, last_error),
                "plain_socket_send(%s:%d), send error code: %d", __FILE__, __LINE__, last_error);
        return BOLT_STATUS_SET;
    }

    *sent = (int) bytes;

    return BOLT_SUCCESS;
}

int plain_socket_recv(BoltCommunication* comm, char* buffer, int length, int* received)
{
    PlainCommunicationContext* context = comm->context;

    ssize_t bytes = socket_recv(context->fd_socket, buffer, (size_t) length, 0);
    if (bytes==-1) {
        int last_error = socket_last_error();
        BoltStatus_set_error_with_ctx(comm->status, socket_transform_error(comm, last_error),
                "plain_socket_recv(%s:%d), recv error code: %d", __FILE__, __LINE__, last_error);
        return BOLT_STATUS_SET;
    }

    if (bytes==0 && length!=0) {
        BoltStatus_set_error_with_ctx(comm->status, BOLT_END_OF_TRANSMISSION,
                "plain_socket_recv(%s:%d), recv returned 0", __FILE__, __LINE__);
        return BOLT_STATUS_SET;
    }

    *received = (int) bytes;

    return BOLT_SUCCESS;
}

int plain_socket_destroy(BoltCommunication* comm)
{
    PlainCommunicationContext* context = comm->context;

    if (context!=NULL) {
        if (context->local_endpoint!=NULL) {
            BoltAddress_destroy(context->local_endpoint);
            context->local_endpoint = NULL;
        }
        if (context->remote_endpoint!=NULL) {
            BoltAddress_destroy(context->remote_endpoint);
            context->remote_endpoint = NULL;
        }

        BoltMem_deallocate(context, sizeof(PlainCommunicationContext));
        comm->context = NULL;
    }

    return BOLT_SUCCESS;
}

BoltAddress* plain_socket_local_endpoint(BoltCommunication* comm)
{
    PlainCommunicationContext* context = comm->context;
    return context->local_endpoint;
}

BoltAddress* plain_socket_remote_endpoint(BoltCommunication* comm)
{
    PlainCommunicationContext* context = comm->context;
    return context->remote_endpoint;
}

int BoltCommunication_startup()
{
    return socket_lifecycle_startup();
}

int BoltCommunication_shutdown()
{
    return socket_lifecycle_shutdown();
}

BoltCommunication*
BoltCommunication_create_plain(BoltSocketOptions* sock_opts, BoltLog* log)
{
    BoltCommunication* comm = BoltMem_allocate(sizeof(BoltCommunication));
    comm->open = &plain_socket_open;
    comm->close = &plain_socket_close;
    comm->send = &plain_socket_send;
    comm->recv = &plain_socket_recv;
    comm->destroy = &plain_socket_destroy;

    comm->get_local_endpoint = &plain_socket_local_endpoint;
    comm->get_remote_endpoint = &plain_socket_remote_endpoint;

    comm->ignore_sigpipe = &plain_socket_ignore_sigpipe;
    comm->restore_sigpipe = &plain_socket_restore_sigpipe;

    comm->last_error = &socket_last_error;
    comm->transform_error = &socket_transform_error;

    comm->status_owned = 1;
    comm->status = BoltStatus_create_with_ctx(1024);
    comm->sock_opts_owned = sock_opts==NULL;
    comm->sock_opts = sock_opts==NULL ? BoltSocketOptions_create() : sock_opts;
    comm->log = log;

    PlainCommunicationContext* context = BoltMem_allocate(sizeof(PlainCommunicationContext));
    context->local_endpoint = NULL;
    context->remote_endpoint = NULL;
    context->fd_socket = 0;
    comm->context = context;

    return comm;
}
