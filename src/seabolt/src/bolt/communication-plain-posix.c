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
#include "status-private.h"

#include <errno.h>

int socket_last_error(BoltCommunication* comm)
{
    UNUSED(comm);
    return errno;
}

int socket_transform_error(BoltCommunication* comm, int error_code)
{
    UNUSED(comm);

    switch (error_code) {
    case EACCES:
    case EPERM:
        return BOLT_PERMISSION_DENIED;
    case EAFNOSUPPORT:
    case EINVAL:
    case EPROTONOSUPPORT:
        return BOLT_UNSUPPORTED;
    case EMFILE:
    case ENFILE:
        return BOLT_OUT_OF_FILES;
    case ENOBUFS:
    case ENOMEM:
        return BOLT_OUT_OF_MEMORY;
    case ECONNREFUSED:
        return BOLT_CONNECTION_REFUSED;
    case ECONNRESET:
        return BOLT_CONNECTION_RESET;
    case EPIPE:
        return BOLT_CONNECTION_RESET;
    case EINTR:
        return BOLT_INTERRUPTED;
    case ENETUNREACH:
        return BOLT_NETWORK_UNREACHABLE;
    case EAGAIN:
    case ETIMEDOUT:
        return BOLT_TIMED_OUT;
    default:
        return BOLT_UNKNOWN_ERROR;
    }
}

int socket_ignore_sigpipe(void** replaced_action)
{
#if !defined(SO_NOSIGPIPE)
    if (*replaced_action==NULL) {
        *replaced_action = malloc(sizeof(struct sigaction));
    }

    struct sigaction ignore_act;
    memset(&ignore_act, '\0', sizeof(ignore_act));
    memset(*replaced_action, '\0', sizeof(struct sigaction));
    ignore_act.sa_handler = SIG_IGN;
    ignore_act.sa_flags = 0;
    return sigaction(SIGPIPE, &ignore_act, *replaced_action);
#endif
    UNUSED(replaced_action);
    return BOLT_SUCCESS;
}

int socket_restore_sigpipe(void** action_to_restore)
{
#if !defined(SO_NOSIGPIPE)
    if (action_to_restore!=NULL) {
        int result = sigaction(SIGPIPE, *action_to_restore, NULL);
        free(*action_to_restore);
        *action_to_restore = NULL;
        return result;
    }
#endif
    UNUSED(action_to_restore);
    return BOLT_SUCCESS;
}

int socket_disable_sigpipe(int sockfd)
{
#if defined(SO_NOSIGPIPE)
    int no = 0;
    return setsockopt(sockfd, SOL_SOCKET, SO_NOSIGPIPE, &no, sizeof(no));
#else
    UNUSED(sockfd);
    return 0;
#endif
}

int socket_set_blocking_mode(int sockfd, int blocking)
{
    const int flags = fcntl(sockfd, F_GETFL, 0);
    if ((flags & O_NONBLOCK) && !blocking) {
        return 0;
    }
    if (!(flags & O_NONBLOCK) && blocking) {
        return 0;
    }

    return fcntl(sockfd, F_SETFL, blocking ? flags ^ O_NONBLOCK : flags | O_NONBLOCK);
}

int socket_set_recv_timeout(int sockfd, int timeout)
{
    struct timeval recv_timeout;
    socklen_t recv_timeout_size = sizeof(struct timeval);

    recv_timeout.tv_sec = timeout/1000;
    recv_timeout.tv_usec = (timeout%1000)*1000;

    return setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &recv_timeout, recv_timeout_size);
}

int socket_set_keepalive(int sockfd, int keepalive)
{
    return setsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE, &keepalive, sizeof(keepalive));
}

int socket_set_nodelay(int sockfd, int nodelay)
{
    return setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, &nodelay, sizeof(nodelay));
}

int socket_open(int domain, int type, int protocol)
{
    return socket(domain, type, protocol);
}

int socket_shutdown(int sockfd)
{
    return shutdown(sockfd, SHUT_RDWR);
}

int socket_close(int sockfd)
{
    return close(sockfd);
}

int socket_connect(int sockfd, const struct sockaddr* addr, socklen_t addrlen, int* in_progress)
{
    int status = connect(sockfd, addr, addrlen);
    *in_progress = status==-1 && errno==EINPROGRESS;
    return status;
}

int socket_send(int sockfd, const void* buf, int len, int flags)
{
    return (int) send(sockfd, buf, len, flags);
}

int socket_recv(int sockfd, void* buf, int len, int flags)
{
    return (int) recv(sockfd, buf, len, flags);
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
    FD_SET(sockfd, &write_set);

    struct timeval select_timeout;
    select_timeout.tv_sec = timeout/1000;
    select_timeout.tv_usec = (timeout%1000)*1000;

    int status = select(FD_SETSIZE, NULL, &write_set, NULL, &select_timeout);
    if (status==1) {
        int so_error = 0;
        socklen_t so_error_len = sizeof(int);

        getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &so_error, &so_error_len);
        if (so_error!=0) {
            errno = so_error;
            return -1;
        }
    }

    return status;
}

int socket_lifecycle_startup()
{
    return 0;
}

int socket_lifecycle_shutdown()
{
    return 0;
}
