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
#include "communication-mock.h"
#include "bolt-private.h"
#include "config-private.h"
#include "mem.h"
#include "name.h"
#include "status-private.h"
#include "log-private.h"

#define MAX_IPADDR_LEN 64

int mock_last_error(BoltCommunication* comm)
{
    UNUSED(comm);
    return BOLT_SUCCESS;
}

int mock_transform_error(BoltCommunication* comm, int error_code)
{
    UNUSED(comm);
    UNUSED(error_code);
    return BOLT_SUCCESS;
}

int mock_socket_ignore_sigpipe(BoltCommunication* comm)
{
    BoltLog_debug(comm->log, "socket_ignore_sigpipe");
    return BOLT_SUCCESS;
}

int mock_socket_restore_sigpipe(BoltCommunication* comm)
{
    BoltLog_debug(comm->log, "socket_restore_sigpipe");
    return BOLT_SUCCESS;
}

int mock_socket_open(BoltCommunication* comm, const struct sockaddr_storage* address)
{
    BoltLog_debug(comm->log, "socket_open");

    MockCommunicationContext* context = comm->context;

    char resolved_host[MAX_IPADDR_LEN], resolved_port[6];
    int status = get_address_components(address, resolved_host, MAX_IPADDR_LEN, resolved_port, 6);
    if (status!=0) {
        BoltStatus_set_error_with_ctx(comm->status, BOLT_ADDRESS_NAME_INFO_FAILED,
                "mock_socket_open(%s:%d), remote get_address_components error code: %d", __FILE__, __LINE__, status);
        return BOLT_STATUS_SET;
    }
    context->remote_endpoint = BoltAddress_create(resolved_host, resolved_port);
    context->local_endpoint = BoltAddress_create("localhost", "65000");

    BoltLog_debug(comm->log, "socket_open: connected");

    return BOLT_SUCCESS;
}

int mock_socket_close(BoltCommunication* comm)
{
    BoltLog_debug(comm->log, "socket_close");

    MockCommunicationContext* context = comm->context;

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

int mock_socket_send(BoltCommunication* comm, char* buffer, int length, int* sent)
{
    UNUSED(buffer);
    BoltLog_debug(comm->log, "socket_send: %d bytes", length);

    *sent = length;

    return BOLT_SUCCESS;
}

int mock_socket_recv(BoltCommunication* comm, char* buffer, int length, int* received)
{
    BoltLog_debug(comm->log, "socket_recv: %d bytes", length);

    MockCommunicationContext* context = comm->context;
    if (!context->protocol_version_sent) {
        memcpy_be(buffer, &context->protocol_version, sizeof(int32_t));
        context->protocol_version_sent = 1;
    }

    *received = length;

    return BOLT_SUCCESS;
}

int mock_socket_destroy(BoltCommunication* comm)
{
    BoltLog_debug(comm->log, "socket_destroy");

    MockCommunicationContext* context = comm->context;

    if (context!=NULL) {
        if (context->local_endpoint!=NULL) {
            BoltAddress_destroy(context->local_endpoint);
            context->local_endpoint = NULL;
        }
        if (context->remote_endpoint!=NULL) {
            BoltAddress_destroy(context->remote_endpoint);
            context->remote_endpoint = NULL;
        }

        BoltMem_deallocate(context, sizeof(MockCommunicationContext));
        comm->context = NULL;
    }

    return BOLT_SUCCESS;
}

BoltAddress* mock_socket_local_endpoint(BoltCommunication* comm)
{
    MockCommunicationContext* context = comm->context;
    return context->local_endpoint;
}

BoltAddress* mock_socket_remote_endpoint(BoltCommunication* comm)
{
    MockCommunicationContext* context = comm->context;
    return context->remote_endpoint;
}

BoltCommunication*
BoltCommunication_create_mock(int32_t version, BoltSocketOptions* sock_opts, BoltLog* log)
{
    BoltCommunication* comm = BoltMem_allocate(sizeof(BoltCommunication));
    comm->open = &mock_socket_open;
    comm->close = &mock_socket_close;
    comm->send = &mock_socket_send;
    comm->recv = &mock_socket_recv;
    comm->destroy = &mock_socket_destroy;

    comm->get_local_endpoint = &mock_socket_local_endpoint;
    comm->get_remote_endpoint = &mock_socket_remote_endpoint;

    comm->ignore_sigpipe = &mock_socket_ignore_sigpipe;
    comm->restore_sigpipe = &mock_socket_restore_sigpipe;

    comm->last_error = &mock_last_error;
    comm->transform_error = &mock_transform_error;

    comm->status_owned = 1;
    comm->status = BoltStatus_create_with_ctx(1024);
    comm->sock_opts_owned = sock_opts==NULL;
    comm->sock_opts = sock_opts==NULL ? BoltSocketOptions_create() : sock_opts;
    comm->log = log;

    MockCommunicationContext* context = BoltMem_allocate(sizeof(MockCommunicationContext));
    context->local_endpoint = NULL;
    context->remote_endpoint = NULL;
    context->protocol_version = version;
    context->protocol_version_sent = 0;

    comm->context = context;

    return comm;
}
