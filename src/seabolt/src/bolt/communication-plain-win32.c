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
#include "communication-secure.h"

int socket_last_error(BoltCommunication* comm)
{
    UNUSED(comm);
    return WSAGetLastError();
}

int socket_transform_error(BoltCommunication* comm, int error_code)
{
    UNUSED(comm);

    switch (error_code) {
    case WSAEACCES:
        return BOLT_PERMISSION_DENIED;
    case WSAEAFNOSUPPORT:
    case WSAEINVAL:
    case WSAEPROTONOSUPPORT:
        return BOLT_UNSUPPORTED;
    case WSAEMFILE:
        return BOLT_OUT_OF_FILES;
    case WSAENOBUFS:
    case WSA_NOT_ENOUGH_MEMORY:
        return BOLT_OUT_OF_MEMORY;
    case WSAECONNREFUSED:
        return BOLT_CONNECTION_REFUSED;
    case WSAEINTR:
        return BOLT_INTERRUPTED;
    case WSAECONNRESET:
        return BOLT_CONNECTION_RESET;
    case WSAENETUNREACH:
    case WSAENETRESET:
    case WSAENETDOWN:
        return BOLT_NETWORK_UNREACHABLE;
    case WSAEWOULDBLOCK:
    case WSAETIMEDOUT:
        return BOLT_TIMED_OUT;
    default:
        return BOLT_UNKNOWN_ERROR;
    }
}

int socket_ignore_sigpipe(void** replaced_action)
{
    UNUSED(replaced_action);
    return BOLT_SUCCESS;
}

int socket_restore_sigpipe(void** action_to_restore)
{
    UNUSED(action_to_restore);
    return BOLT_SUCCESS;
}

int socket_disable_sigpipe(int sockfd)
{
    UNUSED(sockfd);
    return 0;
}

int socket_set_blocking_mode(int sockfd, int blocking)
{
    u_long non_blocking = blocking ? 0 : 1;
    return ioctlsocket(sockfd, FIONBIO, &non_blocking);
}

int socket_set_recv_timeout(int sockfd, int timeout)
{
    int recv_timeout = timeout;
    int recv_timeout_size = sizeof(int);

    return setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*) &recv_timeout, recv_timeout_size);
}

int socket_set_keepalive(int sockfd, int keepalive)
{
    return setsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE, (const char*) &keepalive, sizeof(keepalive));
}

int socket_set_nodelay(int sockfd, int nodelay)
{
    return setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, (const char*) &nodelay, sizeof(nodelay));
}

int socket_open(int domain, int type, int protocol)
{
    return (int) socket(domain, type, protocol);
}

int socket_shutdown(int sockfd)
{
    return shutdown(sockfd, SD_BOTH);
}

int socket_close(int sockfd)
{
    return closesocket(sockfd);
}

int socket_connect(int sockfd, const struct sockaddr* addr, socklen_t addrlen, int* in_progress)
{
    int status = connect(sockfd, addr, addrlen);
    *in_progress = status==-1 && GetLastError()==WSAEWOULDBLOCK;
    return status;
}

int socket_send(int sockfd, const void* buf, int len, int flags)
{
    return send(sockfd, buf, len, flags);
}

int socket_recv(int sockfd, void* buf, int len, int flags)
{
    return recv(sockfd, buf, len, flags);
}

int socket_get_local_addr(int sockfd, struct sockaddr_storage* address, socklen_t* address_size)
{
    return getsockname(sockfd, (struct sockaddr*) address, address_size);
}

int socket_select(int sockfd, int timeout)
{
    // connection in progress
    fd_set write_set;
    FD_ZERO(&write_set);
    FD_SET((SOCKET) sockfd, &write_set);

    struct timeval select_timeout;
    select_timeout.tv_sec = timeout/1000;
    select_timeout.tv_usec = (timeout%1000)*1000;

    int status = select(FD_SETSIZE, NULL, &write_set, NULL, &select_timeout);
    if (status==1) {
        int so_error = 0;
        socklen_t so_error_len = sizeof(int);

        getsockopt(sockfd, SOL_SOCKET, SO_ERROR, (char*) &so_error, &so_error_len);
        if (so_error!=0) {
            WSASetLastError(so_error);
            return -1;
        }
    }

    return status;
}

int socket_lifecycle_startup()
{
    WSADATA data;
    return WSAStartup(MAKEWORD(2, 2), &data);
}

int socket_lifecycle_shutdown()
{
    return WSACleanup();
}
