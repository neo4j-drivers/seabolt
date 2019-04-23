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
#include "bolt-private.h"
#include "communication.h"
#include "mem.h"
#include "name.h"
#include "address-private.h"
#include "config.h"
#include "config-private.h"
#include "log-private.h"
#include "protocol.h"
#include "status-private.h"

#define TRY_COMM(code, status, error_ctx_fmt, file, line) { \
    int status_try = (code); \
    if (status_try != BOLT_SUCCESS) { \
        if (status_try == BOLT_STATUS_SET) { \
            return BOLT_STATUS_SET; \
        } else { \
            BoltStatus_set_error_with_ctx(status, status_try, error_ctx_fmt, file, line, -1); \
        } \
        return status_try; \
    } \
}

void _set_error_with_ctx(BoltStatus* status, int error, const char* error_ctx_format, ...)
{
    status->error = error;
    status->error_ctx[0] = '\0';
    if (error_ctx_format!=NULL) {
        va_list args;
        va_start(args, error_ctx_format);
        vsnprintf(status->error_ctx, status->error_ctx_size, error_ctx_format, args);
        va_end(args);
    }
}

int _open(BoltCommunication* comm, const struct sockaddr_storage* address, const char* id)
{
    switch (address->ss_family) {
    case AF_INET:
    case AF_INET6: {
        char host_string[NI_MAXHOST];
        char port_string[NI_MAXSERV];
        int status = get_address_components(address, host_string, NI_MAXHOST, port_string, NI_MAXSERV);
        if (status==0) {
            BoltLog_info(comm->log, "[%s]: Opening %s connection to %s at port %s", id,
                    address->ss_family==AF_INET ? "IPv4" : "IPv6", &host_string, &port_string);
        }
        break;
    }
    default:
        BoltLog_error(comm->log, "[%s]: Unsupported address family %d", id, address->ss_family);
        return BOLT_UNSUPPORTED;
    }

    TRY_COMM(comm->open(comm, address), comm->status, "_open(%s:%d): unable to establish connection", __FILE__,
            __LINE__);

    return BOLT_SUCCESS;
}

int BoltCommunication_open(BoltCommunication* comm, BoltAddress* address, const char* id)
{
    if (address->n_resolved_hosts==0) {
        return BOLT_ADDRESS_NOT_RESOLVED;
    }

    int status = BOLT_SUCCESS;
    for (int i = 0; i<address->n_resolved_hosts; i++) {
        status = _open(comm, (struct sockaddr_storage*) &address->resolved_hosts[i], id);
        if (status==BOLT_SUCCESS) {
            BoltAddress* remote = comm->get_remote_endpoint(comm);
            BoltLog_info(comm->log, "[%s]: Remote endpoint is %s:%s", id, remote->host, remote->port);

            BoltAddress* local = comm->get_local_endpoint(comm);
            BoltLog_info(comm->log, "[%s]: Local endpoint is %s:%s", id, local->host, local->port);

            BoltStatus_set_error(comm->status, BOLT_SUCCESS);

            break;
        }
    }

    return status;
}

int BoltCommunication_close(BoltCommunication* comm, const char* id)
{
    TRY_COMM(comm->ignore_sigpipe(comm), comm->status, "BoltCommunication_close(%s:%d): unable to ignore SIGPIPE: %d",
            __FILE__, __LINE__);

    BoltLog_debug(comm->log, "[%s]: Closing socket", id);
    int status = comm->close(comm);
    if (status!=BOLT_SUCCESS) {
        _set_error_with_ctx(comm->status, status,
                "BoltCommunication_close(%s:%d), unable to close: %d", __FILE__, __LINE__, status);
        BoltLog_warning(comm->log, "[%s]: Unable to close socket, return code is %d", id, status);
    }

    TRY_COMM(comm->restore_sigpipe(comm), comm->status,
            "BoltCommunication_close(%s:%d): unable to restore SIGPIPE handler: %d",
            __FILE__, __LINE__);

    return status;
}

int BoltCommunication_send(BoltCommunication* comm, char* buffer, int size, const char* id)
{
    if (size==0) {
        return BOLT_SUCCESS;
    }

    TRY_COMM(comm->ignore_sigpipe(comm), comm->status, "BoltCommunication_close(%s:%d): unable to ignore SIGPIPE: %d",
            __FILE__, __LINE__);

    int status = BOLT_SUCCESS;
    int remaining = size;
    int total_sent = 0;
    int sent = 0;
    while (total_sent<size) {
        status = comm->send(comm, buffer+total_sent, remaining, &sent);

        if (status==BOLT_SUCCESS) {
            total_sent += sent;
            remaining -= sent;
        }
        else {
            if (status!=BOLT_STATUS_SET) {
                _set_error_with_ctx(comm->status, status,
                        "BoltCommunication_send(%s:%d), unable to send data: %d", __FILE__, __LINE__, status);

                status = BOLT_STATUS_SET;
            }

            break;
        }
    }

    if (status==BOLT_SUCCESS) {
        BoltLog_info(comm->log, "[%s]: (Sent %d of %d bytes)", id, total_sent, size);
    }

    TRY_COMM(comm->restore_sigpipe(comm), comm->status,
            "BoltCommunication_close(%s:%d): unable to restore SIGPIPE handler: %d", __FILE__, __LINE__);

    return status;
}

int BoltCommunication_receive(BoltCommunication* comm, char* buffer, int min_size, int max_size, int* received,
        const char* id)
{
    if (min_size==0) {
        return BOLT_SUCCESS;
    }

    TRY_COMM(comm->ignore_sigpipe(comm), comm->status, "BoltCommunication_close(%s:%d): unable to ignore SIGPIPE: %d",
            __FILE__, __LINE__);

    int status = BOLT_SUCCESS;
    int max_remaining = max_size;
    int total_received = 0;
    int single_received = 0;
    while (total_received<min_size) {
        status = comm->recv(comm, buffer+total_received, max_remaining, &single_received);

        if (status==BOLT_SUCCESS) {
            total_received += single_received;
            max_remaining -= single_received;
        }
        else {
            if (status!=BOLT_STATUS_SET) {
                _set_error_with_ctx(comm->status, status,
                        "BoltCommunication_receive(%s:%d), unable to receive data: %d", __FILE__, __LINE__, status);

                status = BOLT_STATUS_SET;
            }

            break;
        }
    }

    if (status==BOLT_SUCCESS) {
        if (min_size==max_size) {
            BoltLog_info(comm->log, "[%s]: Received %d of %d bytes", id, total_received, max_size);
        }
        else {
            BoltLog_info(comm->log, "[%s]: Received %d of %d..%d bytes", id, total_received, min_size, max_size);
        }
    }

    *received = total_received;

    TRY_COMM(comm->restore_sigpipe(comm), comm->status,
            "BoltCommunication_close(%s:%d): unable to restore SIGPIPE handler: %d", __FILE__, __LINE__);

    return status;
}

void BoltCommunication_destroy(BoltCommunication* comm)
{
    if (comm->context!=NULL) {
        comm->destroy(comm);
        comm->context = NULL;
    }

    if (comm->status!=NULL && comm->status_owned) {
        BoltStatus_destroy(comm->status);
    }

    if (comm->sock_opts!=NULL && comm->sock_opts_owned) {
        BoltSocketOptions_destroy(comm->sock_opts);
    }

    BoltMem_deallocate(comm, sizeof(BoltCommunication));
}

BoltAddress* BoltCommunication_local_endpoint(BoltCommunication* comm)
{
    return comm->get_local_endpoint(comm);
}

BoltAddress* BoltCommunication_remote_endpoint(BoltCommunication* comm)
{
    return comm->get_remote_endpoint(comm);
}


