/*
 * Copyright (c) 2002-2018 "Neo4j,"
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
#include "address-private.h"
#include "config-private.h"
#include "connection-private.h"
#include "log-private.h"
#include "mem.h"
#include "protocol.h"
#include "tls.h"
#include "v1.h"
#include "v2.h"
#include "v3.h"
#include "platform.h"

#define INITIAL_TX_BUFFER_SIZE 8192
#define INITIAL_RX_BUFFER_SIZE 8192
#define ERROR_CTX_SIZE 1024

#define MAX_IPADDR_LEN 64
#define MAX_ID_LEN 32

#define SOCKET(domain, type, protocol) socket(domain, type, protocol)
#define CONNECT(socket, address, address_size) connect(socket, address, address_size)
#define SHUTDOWN(socket, how) shutdown(socket, how)
#ifdef MSG_NOSIGNAL
#define TRANSMIT(socket, data, size, flags) (int)(send(socket, data, (size_t)(size), flags | MSG_NOSIGNAL))
#define RECEIVE(socket, buffer, size, flags) (int)(recv(socket, buffer, (size_t)(size), flags | MSG_NOSIGNAL))
#else
#define TRANSMIT(socket, data, size, flags) (int)(send(socket, data, (size_t)(size), flags))
#define RECEIVE(socket, buffer, size, flags) (int)(recv(socket, buffer, (size_t)(size), flags))
#endif
#define TRANSMIT_S(socket, data, size, flags) SSL_write(socket, data, size)
#define RECEIVE_S(socket, buffer, size, flags) SSL_read(socket, buffer, size)
#define ADDR_SIZE(address) address->ss_family == AF_INET ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6)
#if USE_WINSOCK
#define GETSOCKETOPT(socket, level, optname, optval, optlen) getsockopt(socket, level, optname, (char *)(optval), optlen)
#define SETSOCKETOPT(socket, level, optname, optval, optlen) setsockopt(socket, level, optname, (const char *)(optval), optlen)
#define CLOSE(socket) closesocket(socket)
#else
#define GETSOCKETOPT(socket, level, optname, optval, optlen) getsockopt(socket, level, optname, (void *)optval, (unsigned int*)optlen)
#define SETSOCKETOPT(socket, level, optname, optval, optlen) setsockopt(socket, level, optname, (const void *)optval, optlen)
#define CLOSE(socket) close(socket)
#endif

#define TRY(code, error_ctx_fmt, file, line) { \
    int status_try = (code); \
    if (status_try != BOLT_SUCCESS) { \
        if (status_try == -1) { \
            int last_error = _last_error_code(); \
            _set_status_with_ctx(connection, BOLT_CONNECTION_STATE_DEFUNCT, _transform_error(last_error), error_ctx_fmt, file, line, last_error); \
        } else if (status_try == BOLT_STATUS_SET) { \
            return -1; \
        } else { \
            _set_status_with_ctx(connection, BOLT_CONNECTION_STATE_DEFUNCT, status_try, error_ctx_fmt, file, line, -1); \
        } \
        return status_try; \
    } \
}

#ifndef SHUT_RDWR
#define SHUT_RDWR SD_BOTH
#endif

static int64_t id_seq = 0;

int _transform_error(int error_code)
{
#if USE_WINSOCK
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
#else
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
#endif
}

int _last_error_code()
{
#if USE_WINSOCK
    return WSAGetLastError();
#else
    return errno;
#endif
}

int _last_error_code_ssl(BoltConnection* connection, int ssl_ret, int* ssl_error_code, int* last_error)
{
    // On windows, SSL_get_error resets WSAGetLastError so we're left without an error code after
    // asking error code - so we're saving it here in case.
    int last_error_saved = _last_error_code();
    *ssl_error_code = SSL_get_error(connection->ssl, ssl_ret);
    switch (*ssl_error_code) {
    case SSL_ERROR_NONE:
        return BOLT_SUCCESS;
    case SSL_ERROR_SYSCALL:
    case SSL_ERROR_WANT_READ:
    case SSL_ERROR_WANT_WRITE: {
        *last_error = _last_error_code();
        if (*last_error==0) {
            *last_error = last_error_saved;
        }
        return _transform_error(*last_error);
    }
    default:
        return BOLT_TLS_ERROR;
    }
}

void _status_changed(BoltConnection* connection)
{
    char* status_text = NULL;
    switch (connection->status->state) {
    case BOLT_CONNECTION_STATE_DISCONNECTED:
        status_text = "<DISCONNECTED>";
        break;
    case BOLT_CONNECTION_STATE_CONNECTED:
        status_text = "<CONNECTED>";
        break;
    case BOLT_CONNECTION_STATE_READY:
        status_text = "<READY>";
        break;
    case BOLT_CONNECTION_STATE_FAILED:
        status_text = "<FAILED>";
        break;
    case BOLT_CONNECTION_STATE_DEFUNCT:
        status_text = "<DEFUNCT>";
        break;
    }

    if (connection->status->error_ctx[0]!=0) {
        BoltLog_info(connection->log, "[%s]: %s [%s]", BoltConnection_id(connection), status_text,
                connection->status->error_ctx);
    }
    else {
        BoltLog_info(connection->log, "[%s]: %s", BoltConnection_id(connection), status_text);
    }

    if (connection->status->state==BOLT_CONNECTION_STATE_DEFUNCT
            || connection->status->state==BOLT_CONNECTION_STATE_FAILED) {
        if (connection->on_error_cb!=NULL) {
            (*connection->on_error_cb)(connection, connection->on_error_cb_state);
        }
    }
}

void _set_status(BoltConnection* connection, BoltConnectionState state, int error)
{
    BoltConnectionState old_status = connection->status->state;
    connection->status->state = state;
    connection->status->error = error;
    connection->status->error_ctx[0] = '\0';

    if (state!=old_status) {
        _status_changed(connection);
    }
}

void _set_status_with_ctx(BoltConnection* connection, BoltConnectionState status, int error,
        const char* error_ctx_format, ...)
{
    BoltConnectionState old_status = connection->status->state;
    connection->status->state = status;
    connection->status->error = error;
    connection->status->error_ctx[0] = '\0';
    if (error_ctx_format!=NULL) {
        va_list args;
        va_start(args, error_ctx_format);
        vsnprintf(connection->status->error_ctx, ERROR_CTX_SIZE, error_ctx_format, args);
        va_end(args);
    }

    if (status!=old_status) {
        _status_changed(connection);
    }
}

int _socket_configure(BoltConnection* connection)
{
    const int YES = 1, NO = 0;

    // Enable TCP_NODELAY
    TRY(SETSOCKETOPT(connection->socket, IPPROTO_TCP, TCP_NODELAY, &YES, sizeof(YES)),
            "_socket_configure(%s:%d), set tcp_nodelay error code: %d", __FILE__, __LINE__);

#ifdef SO_NOSIGPIPE
    // Disable PIPE signals
    TRY(SETSOCKETOPT(connection->socket, SOL_SOCKET, SO_NOSIGPIPE, &NO, sizeof(NO)),
            "_socket_configure(%s:%d), set so_nosigpipe error code: %d", __FILE__, __LINE__);
#endif

    // Set keep-alive accordingly, default: TRUE
    if (connection->sock_opts==NULL || connection->sock_opts->keep_alive) {
        TRY(SETSOCKETOPT(connection->socket, SOL_SOCKET, SO_KEEPALIVE, &YES, sizeof(YES)),
                "_socket_configure(%s:%d), set so_keepalive error code: %d", __FILE__, __LINE__);
    }
    else {
        TRY(SETSOCKETOPT(connection->socket, SOL_SOCKET, SO_KEEPALIVE, &NO, sizeof(NO)),
                "_socket_configure(%s:%d), set so_keepalive error code: %d", __FILE__, __LINE__);
    }

#if USE_WINSOCK
    int recv_timeout, send_timeout;
    int recv_timeout_size = sizeof(int), send_timeout_size = sizeof(int);
    if (connection->sock_opts!=NULL) {
        recv_timeout = connection->sock_opts->recv_timeout;
        send_timeout = connection->sock_opts->send_timeout;
    }
    else {
        recv_timeout = 0;
        send_timeout = 0;
    }
#else
    // Set send and receive timeouts
    struct timeval recv_timeout, send_timeout;
    int recv_timeout_size = sizeof(struct timeval), send_timeout_size = sizeof(struct timeval);
    if (connection->sock_opts!=NULL) {
        recv_timeout.tv_sec = connection->sock_opts->recv_timeout/1000;
        recv_timeout.tv_usec = (connection->sock_opts->recv_timeout%1000)*1000;
        send_timeout.tv_sec = connection->sock_opts->send_timeout/1000;
        send_timeout.tv_usec = (connection->sock_opts->send_timeout%1000)*1000;
    }
    else {
        recv_timeout.tv_sec = 0;
        recv_timeout.tv_usec = 0;
        send_timeout.tv_sec = 0;
        send_timeout.tv_usec = 0;
    }
#endif

    TRY(SETSOCKETOPT(connection->socket, SOL_SOCKET, SO_RCVTIMEO, &recv_timeout, recv_timeout_size),
            "_socket_configure(%s:%d), set so_rcvtimeo error code: %d", __FILE__, __LINE__);
    TRY(SETSOCKETOPT(connection->socket, SOL_SOCKET, SO_SNDTIMEO, &send_timeout, send_timeout_size),
            "_socket_configure(%s:%d), set so_sndtimeo error code: %d", __FILE__, __LINE__);

    return BOLT_SUCCESS;
}

int _socket_set_mode(BoltConnection* connection, int blocking)
{
    int status = 0;

#if USE_WINSOCK
    u_long non_blocking = blocking ? 0 : 1;
    status = ioctlsocket(connection->socket, FIONBIO, &non_blocking);
#else
    const int flags = fcntl(connection->socket, F_GETFL, 0);
    if ((flags & O_NONBLOCK) && !blocking) {
        return 0;
    }
    if (!(flags & O_NONBLOCK) && blocking) {
        return 0;
    }
    status = fcntl(connection->socket, F_SETFL, blocking ? flags ^ O_NONBLOCK : flags | O_NONBLOCK);
#endif

    return status;
}

int _open(BoltConnection* connection, BoltTransport transport, const struct sockaddr_storage* address)
{
    memset(connection->metrics, 0, sizeof(BoltConnectionMetrics));
    connection->transport = transport;
    switch (address->ss_family) {
    case AF_INET:
    case AF_INET6: {
        char host_string[NI_MAXHOST];
        char port_string[NI_MAXSERV];
        getnameinfo((const struct sockaddr*) (address), ADDR_SIZE(address),
                host_string, NI_MAXHOST, port_string, NI_MAXSERV, NI_NUMERICHOST | NI_NUMERICSERV);
        BoltLog_info(connection->log, "[%s]: Opening %s connection to %s at port %s", BoltConnection_id(connection),
                address->ss_family==AF_INET ? "IPv4" : "IPv6", &host_string, &port_string);
        break;
    }
    default:
        BoltLog_error(connection->log, "[%s]: Unsupported address family %d", BoltConnection_id(connection),
                address->ss_family);
        _set_status_with_ctx(connection, BOLT_CONNECTION_STATE_DEFUNCT, BOLT_UNSUPPORTED, "_open(%s:%d)", __FILE__,
                __LINE__);
        return BOLT_STATUS_SET;
    }
    connection->socket = (int) SOCKET(address->ss_family, SOCK_STREAM, IPPROTO_TCP);
    if (connection->socket==-1) {
        int last_error_code = _last_error_code();
        _set_status_with_ctx(connection, BOLT_CONNECTION_STATE_DEFUNCT, _transform_error(last_error_code),
                "_open(%s:%d), socket error code: %d", __FILE__, __LINE__, last_error_code);
        return BOLT_STATUS_SET;
    }

    if (connection->sock_opts!=NULL && connection->sock_opts->connect_timeout>0) {
        // enable non-blocking mode
        TRY(_socket_set_mode(connection, 0), "_open(%s:%d), _socket_set_mode error code: %d", __FILE__, __LINE__);

        // initiate connection
        int status = CONNECT(connection->socket, (struct sockaddr*) (address), ADDR_SIZE(address));
        if (status==-1) {
            int error_code = _last_error_code();
            switch (error_code) {
#if USE_WINSOCK
                case WSAEWOULDBLOCK: {
                    break;
                }
#else
            case EINPROGRESS: {
                break;
            }
#endif
            default:
                _set_status_with_ctx(connection, BOLT_CONNECTION_STATE_DEFUNCT, _transform_error(error_code),
                        "_open(%s:%d), connect error code: %d", __FILE__, __LINE__, error_code);
                return BOLT_STATUS_SET;
            }
        }

        if (status) {
            // connection in progress
            fd_set write_set;
            FD_ZERO(&write_set);
            FD_SET(connection->socket, &write_set);

            struct timeval timeout;
            timeout.tv_sec = connection->sock_opts->connect_timeout/1000;
            timeout.tv_usec = (connection->sock_opts->connect_timeout%1000)*1000;

            status = select(FD_SETSIZE, NULL, &write_set, NULL, &timeout);
            switch (status) {
            case 0: {
                //timeout expired
                BoltLog_info(connection->log, "[%s]: Connect timed out", BoltConnection_id(connection));
                _set_status_with_ctx(connection, BOLT_CONNECTION_STATE_DEFUNCT, BOLT_TIMED_OUT, "_open(%s:%d)",
                        __FILE__, __LINE__);
                return BOLT_STATUS_SET;
            }
            case 1: {
                int so_error;
                int so_error_len = sizeof(int);

                TRY(GETSOCKETOPT(connection->socket, SOL_SOCKET, SO_ERROR, &so_error, &so_error_len),
                        "_open(%s:%d), getsocketopt error code: %d", __FILE__, __LINE__);
                if (so_error!=0) {
                    _set_status_with_ctx(connection, BOLT_CONNECTION_STATE_DEFUNCT, _transform_error(so_error),
                            "_open(%s:%d), socket error code: %d", __FILE__, __LINE__, so_error);
                    return BOLT_STATUS_SET;
                }

                break;
            }
            default: {
                int last_error = _last_error_code();
                _set_status_with_ctx(connection, BOLT_CONNECTION_STATE_DEFUNCT, _transform_error(last_error),
                        "_open(%s:%d), select error code: %d", __FILE__, __LINE__, last_error);
                return BOLT_STATUS_SET;
            }
            }
        }

        // revert to blocking mode
        TRY(_socket_set_mode(connection, 1), "_open(%s:%d), _socket_set_mode error code: %d", __FILE__, __LINE__);
    }
    else {
        TRY(CONNECT(connection->socket, (struct sockaddr*) (address), ADDR_SIZE(address)),
                "_open(%s:%d), connect error code: %d", __FILE__, __LINE__);
    }

    TRY(_socket_configure(connection), "_open(%s:%d), _socket_configure error code: %d", __FILE__, __LINE__);
    BoltUtil_get_time(&connection->metrics->time_opened);
    connection->tx_buffer = BoltBuffer_create(INITIAL_TX_BUFFER_SIZE);
    connection->rx_buffer = BoltBuffer_create(INITIAL_RX_BUFFER_SIZE);
    return BOLT_SUCCESS;
}

int _secure(BoltConnection* connection, struct BoltTrust* trust)
{
    int status = 1;

    BoltLog_info(connection->log, "[%s]: Securing socket", BoltConnection_id(connection));

    if (connection->ssl_context==NULL) {
        connection->ssl_context = create_ssl_ctx(trust, connection->address->host, connection->log, connection->id);
        connection->owns_ssl_context = 1;
    }

    connection->ssl = SSL_new(connection->ssl_context);

    // Link to underlying socket
    if (status) {
        status = SSL_set_fd(connection->ssl, connection->socket);

        if (!status) {
            _set_status_with_ctx(connection, BOLT_CONNECTION_STATE_DEFUNCT, BOLT_TLS_ERROR,
                    "_secure(%s:%d), SSL_set_fd returned: %d", __FILE__, __LINE__, status);

        }
    }

    // Enable SNI
    if (status) {
        status = SSL_set_tlsext_host_name(connection->ssl, connection->address->host);

        if (!status) {
            _set_status_with_ctx(connection, BOLT_CONNECTION_STATE_DEFUNCT, BOLT_TLS_ERROR,
                    "_secure(%s:%d), SSL_set_tlsext_host_name returned: %d", __FILE__, __LINE__, status);

        }
    }

    if (status) {
        status = SSL_connect(connection->ssl);
        if (status==1) {
            return 0;
        }

        _set_status_with_ctx(connection, BOLT_CONNECTION_STATE_DEFUNCT, BOLT_TLS_ERROR,
                "_secure(%s:%d), SSL_connect returned: %d",
                __FILE__, __LINE__, status);
    }

    SSL_free(connection->ssl);
    if (connection->owns_ssl_context) {
        SSL_CTX_free(connection->ssl_context);
    }
    connection->owns_ssl_context = 0;
    connection->ssl_context = NULL;
    connection->ssl = NULL;

    return -1;
}

void _close(BoltConnection* connection)
{
    BoltLog_info(connection->log, "[%s]: Closing connection", BoltConnection_id(connection));

    if (connection->protocol!=NULL) {
        connection->protocol->goodbye(connection);

        switch (connection->protocol_version) {
        case 1:
            BoltProtocolV1_destroy_protocol(connection->protocol);
            break;
        case 2:
            BoltProtocolV2_destroy_protocol(connection->protocol);
            break;
        case 3:
            BoltProtocolV3_destroy_protocol(connection->protocol);
            break;
        default:
            break;
        }
        connection->protocol = NULL;
        connection->protocol_version = 0;
    }

#if USE_POSIXSOCK && !defined(SO_NOSIGPIPE)
    struct sigaction ignore_act, previous_act;
    memset(&ignore_act, '\0', sizeof(ignore_act));
    memset(&previous_act, '\0', sizeof(previous_act));
    ignore_act.sa_handler = SIG_IGN;
    ignore_act.sa_flags = 0;
    if (sigaction(SIGPIPE, &ignore_act, &previous_act)<0) {
        int last_error = _last_error_code();
        _set_status_with_ctx(connection, BOLT_CONNECTION_STATE_DEFUNCT, _transform_error(last_error),
                "_send(%s:%d), unable to install ignore handler for SIGPIPE: %d", __FILE__, __LINE__,
                last_error);
    }
#endif

    switch (connection->transport) {
    case BOLT_TRANSPORT_PLAINTEXT: {
        SHUTDOWN(connection->socket, SHUT_RDWR);
        CLOSE(connection->socket);
        break;
    }
    case BOLT_TRANSPORT_ENCRYPTED: {
        if (connection->ssl!=NULL) {
            SSL_shutdown(connection->ssl);
            SSL_free(connection->ssl);
            connection->ssl = NULL;
        }
        if (connection->ssl_context!=NULL && connection->owns_ssl_context) {
            SSL_CTX_free(connection->ssl_context);
            connection->ssl_context = NULL;
            connection->owns_ssl_context = 0;
        }
        SHUTDOWN(connection->socket, SHUT_RDWR);
        CLOSE(connection->socket);
        break;
    }
    }
    BoltUtil_get_time(&connection->metrics->time_closed);
    connection->socket = 0;
    _set_status(connection, BOLT_CONNECTION_STATE_DISCONNECTED, BOLT_SUCCESS);

#if USE_POSIXSOCK && !defined(SO_NOSIGPIPE)
    if (sigaction(SIGPIPE, &previous_act, NULL)<0) {
        int last_error = _last_error_code();
        _set_status_with_ctx(connection, BOLT_CONNECTION_STATE_DEFUNCT, _transform_error(last_error),
                "_send(%s:%d), unable to revert to previous handler for SIGPIPE: %d", __FILE__, __LINE__,
                last_error);
    }
#endif
}

int _send(BoltConnection* connection, const char* data, int size)
{
    if (size==0) {
        return BOLT_SUCCESS;
    }

#if USE_POSIXSOCK && !defined(SO_NOSIGPIPE)
    struct sigaction ignore_act, previous_act;
#if defined(MSG_NOSIGNAL)
    if (connection->transport==BOLT_TRANSPORT_ENCRYPTED) {
#endif
        memset(&ignore_act, '\0', sizeof(ignore_act));
        memset(&previous_act, '\0', sizeof(previous_act));
        ignore_act.sa_handler = SIG_IGN;
        ignore_act.sa_flags = 0;
        if (sigaction(SIGPIPE, &ignore_act, &previous_act)<0) {
            int last_error = _last_error_code();
            _set_status_with_ctx(connection, BOLT_CONNECTION_STATE_DEFUNCT, _transform_error(last_error),
                    "_send(%s:%d), unable to install ignore handler for SIGPIPE: %d", __FILE__, __LINE__,
                    last_error);
            return BOLT_STATUS_SET;
        }
#if defined(MSG_NOSIGNAL)
    }
#endif
#endif
    int status = BOLT_SUCCESS;
    int remaining = size;
    int total_sent = 0;
    while (total_sent<size) {
        int sent = 0;
        switch (connection->transport) {
        case BOLT_TRANSPORT_PLAINTEXT: {
            sent = TRANSMIT(connection->socket, data+total_sent, remaining, 0);
            break;
        }
        case BOLT_TRANSPORT_ENCRYPTED: {
            sent = TRANSMIT_S(connection->ssl, data+total_sent, remaining, 0);
            break;
        }
        }

        if (sent>=0) {
            connection->metrics->bytes_sent += sent;
            total_sent += sent;
            remaining -= sent;
        }
        else {
            switch (connection->transport) {
            case BOLT_TRANSPORT_PLAINTEXT: {
                int last_error = _last_error_code();
                _set_status_with_ctx(connection, BOLT_CONNECTION_STATE_DEFUNCT, _transform_error(last_error),
                        "_send(%s:%d), send error code: %d", __FILE__, __LINE__, last_error);
                BoltLog_error(connection->log, "[%s]: Socket error %d on transmit", BoltConnection_id(connection),
                        connection->status->error);
                break;
            }
            case BOLT_TRANSPORT_ENCRYPTED: {
                int last_error = 0, ssl_error = 0;
                int last_error_transformed = _last_error_code_ssl(connection, sent, &ssl_error, &last_error);
                _set_status_with_ctx(connection, BOLT_CONNECTION_STATE_DEFUNCT, last_error_transformed,
                        "_send(%s:%d), send_s error code: %d, underlying error code: %d", __FILE__, __LINE__, ssl_error,
                        last_error);
                BoltLog_error(connection->log, "[%s]: SSL error %d on transmit", BoltConnection_id(connection),
                        connection->status->error);
                break;
            }
            }

            status = BOLT_TRANSPORT_UNSUPPORTED;
            break;
        }
    }
    if (status==BOLT_SUCCESS) {
        BoltLog_info(connection->log, "[%s]: (Sent %d of %d bytes)", BoltConnection_id(connection), total_sent, size);
    }
#if USE_POSIXSOCK && !defined(SO_NOSIGPIPE)
#if defined(MSG_NOSIGNAL)
    if (connection->transport==BOLT_TRANSPORT_ENCRYPTED) {
#endif
        if (sigaction(SIGPIPE, &previous_act, NULL)<0) {
            int last_error = _last_error_code();
            _set_status_with_ctx(connection, BOLT_CONNECTION_STATE_DEFUNCT, _transform_error(last_error),
                    "_send(%s:%d), unable to revert to previous handler for SIGPIPE: %d", __FILE__, __LINE__,
                    last_error);
            return BOLT_STATUS_SET;
        }
#if defined(MSG_NOSIGNAL)
    }
#endif
#endif
    return status;
}

/**
 * Attempt to receive between min_size and max_size bytes.
 *
 * @param connection
 * @param buffer
 * @param min_size
 * @param max_size
 * @return
 */
int _receive(BoltConnection* connection, char* buffer, int min_size, int max_size, int* received)
{
    if (min_size==0) {
        return BOLT_SUCCESS;
    }

#if USE_POSIXSOCK && !defined(SO_NOSIGPIPE)
    struct sigaction ignore_act, previous_act;
#if defined(MSG_NOSIGNAL)
    if (connection->transport==BOLT_TRANSPORT_ENCRYPTED) {
#endif
        memset(&ignore_act, '\0', sizeof(ignore_act));
        memset(&previous_act, '\0', sizeof(previous_act));
        ignore_act.sa_handler = SIG_IGN;
        ignore_act.sa_flags = 0;
        if (sigaction(SIGPIPE, &ignore_act, &previous_act)<0) {
            int last_error = _last_error_code();
            _set_status_with_ctx(connection, BOLT_CONNECTION_STATE_DEFUNCT, _transform_error(last_error),
                    "_send(%s:%d), unable to install ignore handler for SIGPIPE: %d", __FILE__, __LINE__,
                    last_error);
            return BOLT_STATUS_SET;
        }
#if defined(MSG_NOSIGNAL)
    }
#endif
#endif

    int status = BOLT_SUCCESS;
    int max_remaining = max_size;
    int total_received = 0;
    while (total_received<min_size) {
        int single_received = 0;
        switch (connection->transport) {
        case BOLT_TRANSPORT_PLAINTEXT:
            single_received = RECEIVE(connection->socket, buffer+total_received, max_remaining, 0);
            break;
        case BOLT_TRANSPORT_ENCRYPTED:
            single_received = RECEIVE_S(connection->ssl, buffer+total_received, max_remaining, 0);
            break;
        }
        if (single_received>0) {
            connection->metrics->bytes_received += single_received;
            total_received += single_received;
            max_remaining -= single_received;
        }
        else if (single_received==0) {
            BoltLog_info(connection->log, "[%s]: Detected end of transmission", BoltConnection_id(connection));
            _set_status_with_ctx(connection, BOLT_CONNECTION_STATE_DISCONNECTED, BOLT_END_OF_TRANSMISSION,
                    "_receive(%s:%d), receive returned 0", __FILE__, __LINE__);
            status = BOLT_STATUS_SET;
            break;
        }
        else {
            switch (connection->transport) {
            case BOLT_TRANSPORT_PLAINTEXT: {
                int last_error = _last_error_code();
                _set_status_with_ctx(connection, BOLT_CONNECTION_STATE_DEFUNCT, _transform_error(last_error),
                        "_receive(%s:%d), receive error code: %d", __FILE__, __LINE__, last_error);
                BoltLog_error(connection->log, "[%s]: Socket error %d on receive", BoltConnection_id(connection),
                        connection->status->error);
                break;
            }
            case BOLT_TRANSPORT_ENCRYPTED: {
                int last_error = 0, ssl_error = 0;
                int last_error_transformed = _last_error_code_ssl(connection, single_received, &ssl_error,
                        &last_error);
                _set_status_with_ctx(connection, BOLT_CONNECTION_STATE_DEFUNCT, last_error_transformed,
                        "_receive(%s:%d), receive_s error code: %d, underlying error code: %d", __FILE__, __LINE__,
                        ssl_error, last_error);
                BoltLog_error(connection->log, "[%s]: SSL error %d on receive", BoltConnection_id(connection),
                        connection->status->error);
                break;
            }
            }
            status = BOLT_STATUS_SET;
            break;
        }
    }
    if (status==BOLT_SUCCESS) {
        if (min_size==max_size) {
            BoltLog_info(connection->log, "[%s]: Received %d of %d bytes", BoltConnection_id(connection),
                    total_received,
                    max_size);
        }
        else {
            BoltLog_info(connection->log, "[%s]: Received %d of %d..%d bytes", BoltConnection_id(connection),
                    total_received, min_size, max_size);
        }
    }
    *received = total_received;
#if USE_POSIXSOCK && !defined(SO_NOSIGPIPE)
#if defined(MSG_NOSIGNAL)
    if (connection->transport==BOLT_TRANSPORT_ENCRYPTED) {
#endif
        if (sigaction(SIGPIPE, &previous_act, NULL)<0) {
            int last_error = _last_error_code();
            _set_status_with_ctx(connection, BOLT_CONNECTION_STATE_DEFUNCT, _transform_error(last_error),
                    "_send(%s:%d), unable to revert to previous handler for SIGPIPE: %d", __FILE__, __LINE__,
                    last_error);
            return BOLT_STATUS_SET;
        }
#if defined(MSG_NOSIGNAL)
    }
#endif
#endif
    return status;
}

int handshake_b(BoltConnection* connection, int32_t _1, int32_t _2, int32_t _3, int32_t _4)
{
    BoltLog_info(connection->log, "[%s]: Performing handshake", BoltConnection_id(connection));
    char handshake[20];
    memcpy(&handshake[0x00], "\x60\x60\xB0\x17", 4);
    memcpy_be(&handshake[0x04], &_1, 4);
    memcpy_be(&handshake[0x08], &_2, 4);
    memcpy_be(&handshake[0x0C], &_3, 4);
    memcpy_be(&handshake[0x10], &_4, 4);
    TRY(_send(connection, &handshake[0], 20), "handshake_b(%s:%d), _send error code: %d", __FILE__, __LINE__);
    int received = 0;
    TRY(_receive(connection, &handshake[0], 4, 4, &received), "handshake_b(%s:%d), _receive error code: %d", __FILE__,
            __LINE__);
    memcpy_be(&connection->protocol_version, &handshake[0], 4);
    BoltLog_info(connection->log, "[%s]: <SET protocol_version=%d>", BoltConnection_id(connection),
            connection->protocol_version);
    switch (connection->protocol_version) {
    case 1:
        connection->protocol = BoltProtocolV1_create_protocol(connection);
        return BOLT_SUCCESS;
    case 2:
        connection->protocol = BoltProtocolV2_create_protocol(connection);
        return BOLT_SUCCESS;
    case 3:
        connection->protocol = BoltProtocolV3_create_protocol(connection);
        return BOLT_SUCCESS;
    default:
        _close(connection);
        return BOLT_PROTOCOL_UNSUPPORTED;
    }
}

BoltConnection* BoltConnection_create()
{
    const size_t size = sizeof(BoltConnection);
    BoltConnection* connection = BoltMem_allocate(size);
    memset(connection, 0, size);
    connection->status = BoltStatus_create_with_ctx(ERROR_CTX_SIZE);
    connection->metrics = BoltMem_allocate(sizeof(BoltConnectionMetrics));
    return connection;
}

void BoltConnection_destroy(BoltConnection* connection)
{
    if (connection->status!=NULL) {
        BoltStatus_destroy(connection->status);
    }
    if (connection->metrics!=NULL) {
        BoltMem_deallocate(connection->metrics, sizeof(BoltConnectionMetrics));
    }
    BoltMem_deallocate(connection, sizeof(BoltConnection));
}

int32_t
BoltConnection_open(BoltConnection* connection, BoltTransport transport, struct BoltAddress* address,
        struct BoltTrust* trust, struct BoltLog* log, struct BoltSocketOptions* sock_opts)
{
    if (connection->status!=BOLT_CONNECTION_STATE_DISCONNECTED) {
        BoltConnection_close(connection);
    }
    // Id buffer composed of local&remote Endpoints
    connection->id = BoltMem_allocate(MAX_ID_LEN);
    snprintf(connection->id, MAX_ID_LEN, "conn-%" PRId64, BoltUtil_increment(&id_seq));
    connection->log = log;
    // Store connection info
    connection->address = BoltAddress_create(address->host, address->port);
    // Store socket options
    connection->sock_opts = sock_opts;
    for (int i = 0; i<address->n_resolved_hosts; i++) {
        const int opened = _open(connection, transport, &address->resolved_hosts[i]);
        if (opened==BOLT_SUCCESS) {
            if (connection->transport==BOLT_TRANSPORT_ENCRYPTED) {
                const int secured = _secure(connection, trust);
                if (secured==0) {
                    TRY(handshake_b(connection, 3, 2, 1, 0), "BoltConnection_open(%s:%d), handshake_b error code: %d",
                            __FILE__, __LINE__);
                }
                else {
                    return connection->status->error;
                }
            }
            else {
                TRY(handshake_b(connection, 3, 2, 1, 0), "BoltConnection_open(%s:%d), handshake_b error code: %d",
                        __FILE__, __LINE__);
            }

            char resolved_host[MAX_IPADDR_LEN], resolved_port[6];
            int resolved_family = BoltAddress_copy_resolved_host(address, i, resolved_host, MAX_IPADDR_LEN);
            sprintf(resolved_port, "%d", address->resolved_port);
            connection->resolved_address = BoltAddress_create(resolved_host, resolved_port);
            BoltLog_info(connection->log, "[%s]: Remote endpoint is %s:%s", BoltConnection_id(connection),
                    resolved_host, resolved_port);

            struct sockaddr_storage local_addr;
            socklen_t local_addr_size =
                    resolved_family==AF_INET ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6);
            TRY(getsockname(connection->socket, (struct sockaddr*) &local_addr, &local_addr_size),
                    "BoltConnection_open(%s:%d), getsockname error code: %d", __FILE__, __LINE__);

            char local_host[MAX_IPADDR_LEN], local_port[6];
            TRY(getnameinfo((const struct sockaddr*) &local_addr, local_addr_size, local_host, MAX_IPADDR_LEN,
                    local_port, 6, NI_NUMERICHOST | NI_NUMERICSERV),
                    "BoltConnection_open(%s:%d), getnameinfo error code: %d", __FILE__, __LINE__);
            connection->local_address = BoltAddress_create(local_host, local_port);
            BoltLog_info(connection->log, "[%s]: Local endpoint is %s:%s", BoltConnection_id(connection), local_host,
                    local_port);

            _set_status(connection, BOLT_CONNECTION_STATE_CONNECTED, BOLT_SUCCESS);
            break;
        }
    }
    if (connection->status->state==BOLT_CONNECTION_STATE_DISCONNECTED) {
        _set_status_with_ctx(connection, BOLT_CONNECTION_STATE_DEFUNCT, BOLT_NO_VALID_ADDRESS,
                "BoltConnection_open(%s:%d)", __FILE__,
                __LINE__);
        return -1;
    }
    return connection->status->state==BOLT_CONNECTION_STATE_READY ? BOLT_SUCCESS : connection->status->error;
}

void BoltConnection_close(BoltConnection* connection)
{
    if (connection->status->state!=BOLT_CONNECTION_STATE_DISCONNECTED) {
        _close(connection);
    }
    if (connection->rx_buffer!=NULL) {
        BoltBuffer_destroy(connection->rx_buffer);
        connection->rx_buffer = NULL;
    }
    if (connection->tx_buffer!=NULL) {
        BoltBuffer_destroy(connection->tx_buffer);
        connection->tx_buffer = NULL;
    }
    if (connection->address!=NULL) {
        BoltAddress_destroy((struct BoltAddress*) connection->address);
        connection->address = NULL;
    }
    if (connection->resolved_address!=NULL) {
        BoltAddress_destroy((struct BoltAddress*) connection->resolved_address);
        connection->resolved_address = NULL;
    }
    if (connection->local_address!=NULL) {
        BoltAddress_destroy((struct BoltAddress*) connection->local_address);
        connection->local_address = NULL;
    }
    if (connection->id!=NULL) {
        BoltMem_deallocate(connection->id, MAX_ID_LEN);
        connection->id = NULL;
    }
}

int32_t BoltConnection_send(BoltConnection* connection)
{
    int size = BoltBuffer_unloadable(connection->tx_buffer);
    TRY(_send(connection, BoltBuffer_unload_pointer(connection->tx_buffer, size), size),
            "BoltConnection_send(%s:%d), _send error code: %d", __FILE__, __LINE__);
    BoltBuffer_compact(connection->tx_buffer);
    return 0;
}

int BoltConnection_receive(BoltConnection* connection, char* buffer, int size)
{
    if (size==0) return 0;
    int available = BoltBuffer_unloadable(connection->rx_buffer);
    if (size>available) {
        int delta = size-available;
        while (delta>0) {
            int max_size = BoltBuffer_loadable(connection->rx_buffer);
            if (max_size==0) {
                BoltBuffer_compact(connection->rx_buffer);
                max_size = BoltBuffer_loadable(connection->rx_buffer);
            }
            max_size = delta>max_size ? delta : max_size;
            int received = 0;
            TRY(_receive(connection, BoltBuffer_load_pointer(connection->rx_buffer, max_size), delta, max_size,
                    &received), "BoltConnection_receive(%s:%d), _receive error code: %d", __FILE__, __LINE__);
            // adjust the buffer extent based on the actual amount of data received
            connection->rx_buffer->extent = connection->rx_buffer->extent-max_size+received;
            delta -= received;
        }
    }
    BoltBuffer_unload(connection->rx_buffer, buffer, size);
    return BOLT_SUCCESS;
}

int BoltConnection_fetch(BoltConnection* connection, BoltRequest request)
{
    const int fetched = connection->protocol->fetch(connection, request);
    if (fetched==FETCH_SUMMARY) {
        if (connection->protocol->is_success_summary(connection)) {
            _set_status(connection, BOLT_CONNECTION_STATE_READY, BOLT_SUCCESS);
        }
        else if (connection->protocol->is_ignored_summary(connection)) {
            // we may need to update status based on an earlier reported FAILURE
            // which our consumer did not care its result
            if (connection->protocol->failure(connection)!=NULL) {
                _set_status_with_ctx(connection, BOLT_CONNECTION_STATE_FAILED, BOLT_SERVER_FAILURE,
                        "BoltConnection_fetch(%s:%d), failure upon ignored message", __FILE__, __LINE__);
            }
        }
        else if (connection->protocol->is_failure_summary(connection)) {
            _set_status_with_ctx(connection, BOLT_CONNECTION_STATE_FAILED, BOLT_SERVER_FAILURE,
                    "BoltConnection_fetch(%s:%d), failure message", __FILE__, __LINE__);
        }
        else {
            BoltLog_error(connection->log, "[%s]: Protocol violation (received summary code %d)",
                    BoltConnection_id(connection), connection->protocol->last_data_type(connection));
            _set_status_with_ctx(connection, BOLT_CONNECTION_STATE_DEFUNCT, BOLT_PROTOCOL_VIOLATION,
                    "BoltConnection_fetch(%s:%d), received summary code: %d", __FILE__, __LINE__,
                    connection->protocol->last_data_type(connection));
            return FETCH_ERROR;
        }
    }
    return fetched;
}

int32_t BoltConnection_fetch_summary(BoltConnection* connection, BoltRequest request)
{
    int records = 0;
    int data;
    do {
        data = BoltConnection_fetch(connection, request);
        if (data<0) {
            return data;
        }
        records += data;
    }
    while (data);
    return records;
}

struct BoltValue* BoltConnection_field_values(BoltConnection* connection)
{
    return connection->protocol->field_values(connection);
}

int32_t BoltConnection_summary_success(BoltConnection* connection)
{
    return connection->protocol->is_success_summary(connection);
}

int32_t BoltConnection_summary_failure(BoltConnection* connection)
{
    return connection->protocol->is_failure_summary(connection);
}

int32_t
BoltConnection_init(BoltConnection* connection, const char* user_agent, const struct BoltValue* auth_token)
{
    BoltLog_info(connection->log, "[%s]: Initialising connection", BoltConnection_id(connection));
    switch (connection->protocol_version) {
    case 1:
    case 2:
    case 3: {
        int code = connection->protocol->init(connection, user_agent, auth_token);
        switch (code) {
        case BOLT_V1_SUCCESS:
            _set_status(connection, BOLT_CONNECTION_STATE_READY, BOLT_SUCCESS);
            return 0;
        case BOLT_V1_FAILURE:
            _set_status_with_ctx(connection, BOLT_CONNECTION_STATE_DEFUNCT, BOLT_PERMISSION_DENIED,
                    "BoltConnection_init(%s:%d), failure message", __FILE__, __LINE__);
            return -1;
        default:
            BoltLog_error(connection->log, "[%s]: Protocol violation (received summary code %d)",
                    BoltConnection_id(connection), code);
            _set_status_with_ctx(connection, BOLT_CONNECTION_STATE_DEFUNCT, BOLT_PROTOCOL_VIOLATION,
                    "BoltConnection_init(%s:%d), received summary code: %d", __FILE__, __LINE__, code);
            return -1;
        }
    }
    default:
        _set_status_with_ctx(connection, BOLT_CONNECTION_STATE_DEFUNCT, BOLT_PROTOCOL_UNSUPPORTED,
                "BoltConnection_init(%s:%d)",
                __FILE__, __LINE__);
        return -1;
    }
}

int32_t BoltConnection_clear_begin(BoltConnection* connection)
{
    TRY(connection->protocol->clear_begin_tx(connection), "BoltConnection_clear_begin(%s:%d), error code: %d", __FILE__,
            __LINE__);
    return BOLT_SUCCESS;
}

int32_t BoltConnection_set_begin_bookmarks(BoltConnection* connection, struct BoltValue* bookmark_list)
{
    TRY(connection->protocol->set_begin_tx_bookmark(connection, bookmark_list),
            "BoltConnection_set_begin_bookmarks(%s:%d), error code: %d", __FILE__, __LINE__);
    return BOLT_SUCCESS;
}

int32_t BoltConnection_set_begin_tx_timeout(BoltConnection* connection, int64_t timeout)
{
    TRY(connection->protocol->set_begin_tx_timeout(connection, timeout),
            "BoltConnection_set_begin_tx_timeout(%s:%d), error code: %d", __FILE__, __LINE__);
    return BOLT_SUCCESS;
}

int32_t BoltConnection_set_begin_tx_metadata(BoltConnection* connection, struct BoltValue* metadata)
{
    TRY(connection->protocol->set_begin_tx_metadata(connection, metadata),
            "BoltConnection_set_begin_tx_metadata(%s:%d), error code: %d", __FILE__, __LINE__);
    return BOLT_SUCCESS;
}

int32_t BoltConnection_load_begin_request(BoltConnection* connection)
{
    TRY(connection->protocol->load_begin_tx(connection), "BoltConnection_load_begin_request(%s:%d), error code: %d",
            __FILE__,
            __LINE__);
    return BOLT_SUCCESS;
}

int32_t BoltConnection_load_commit_request(BoltConnection* connection)
{
    TRY(connection->protocol->load_commit_tx(connection), "BoltConnection_load_commit_request(%s:%d), error code: %d",
            __FILE__,
            __LINE__);
    return BOLT_SUCCESS;
}

int32_t BoltConnection_load_rollback_request(BoltConnection* connection)
{
    TRY(connection->protocol->load_rollback_tx(connection),
            "BoltConnection_load_rollback_request(%s:%d), error code: %d",
            __FILE__, __LINE__);
    return BOLT_SUCCESS;
}

int32_t BoltConnection_clear_run(BoltConnection* connection)
{
    TRY(connection->protocol->clear_run(connection), "BoltConnection_clear_run(%s:%d), error code: %d", __FILE__,
            __LINE__);
    return BOLT_SUCCESS;
}

int32_t
BoltConnection_set_run_cypher(BoltConnection* connection, const char* cypher, const uint64_t cypher_size,
        const int32_t n_parameter)
{
    TRY(connection->protocol->set_run_cypher(connection, cypher, cypher_size, n_parameter),
            "BoltConnection_set_run_cypher(%s:%d), error code: %d", __FILE__, __LINE__);
    return BOLT_SUCCESS;
}

struct BoltValue*
BoltConnection_set_run_cypher_parameter(BoltConnection* connection, int32_t index, const char* name,
        const uint64_t name_size)
{
    return connection->protocol->set_run_cypher_parameter(connection, index, name, name_size);
}

int32_t BoltConnection_set_run_bookmarks(BoltConnection* connection, struct BoltValue* bookmark_list)
{
    TRY(connection->protocol->set_run_bookmark(connection, bookmark_list),
            "BoltConnection_set_run_bookmarks(%s:%d), error code: %d", __FILE__, __LINE__);
    return BOLT_SUCCESS;
}

int32_t BoltConnection_set_run_tx_timeout(BoltConnection* connection, int64_t timeout)
{
    TRY(connection->protocol->set_run_tx_timeout(connection, timeout),
            "BoltConnection_set_run_tx_timeout(%s:%d), error code: %d", __FILE__, __LINE__);
    return BOLT_SUCCESS;
}

int32_t BoltConnection_set_run_tx_metadata(BoltConnection* connection, struct BoltValue* metadata)
{
    TRY(connection->protocol->set_run_tx_metadata(connection, metadata),
            "BoltConnection_set_run_tx_metadata(%s:%d), error code: %d", __FILE__, __LINE__);
    return BOLT_SUCCESS;
}

int32_t BoltConnection_load_run_request(BoltConnection* connection)
{
    TRY(connection->protocol->load_run(connection), "BoltConnection_load_run_request(%s:%d), error code: %d", __FILE__,
            __LINE__);
    return BOLT_SUCCESS;
}

int32_t BoltConnection_load_discard_request(BoltConnection* connection, int32_t n)
{
    TRY(connection->protocol->load_discard(connection, n), "BoltConnection_load_discard_request(%s:%d), error code: %d",
            __FILE__, __LINE__);
    return BOLT_SUCCESS;
}

int32_t BoltConnection_load_pull_request(BoltConnection* connection, int32_t n)
{
    TRY(connection->protocol->load_pull(connection, n), "BoltConnection_load_pull_request(%s:%d), error code: %d",
            __FILE__, __LINE__);
    return BOLT_SUCCESS;
}

int32_t BoltConnection_load_reset_request(BoltConnection* connection)
{
    TRY(connection->protocol->load_reset(connection), "BoltConnection_load_reset_request(%s:%d), error code: %d",
            __FILE__, __LINE__);
    return BOLT_SUCCESS;
}

BoltRequest BoltConnection_last_request(BoltConnection* connection)
{
    return connection->protocol->last_request(connection);
}

const char* BoltConnection_server(BoltConnection* connection)
{
    return connection->protocol->server(connection);
}

const char* BoltConnection_id(BoltConnection* connection)
{
    if (connection->protocol!=NULL && connection->protocol->id!=NULL) {
        return connection->protocol->id(connection);
    }

    return connection->id;
}

const BoltAddress* BoltConnection_address(BoltConnection* connection)
{
    return connection->address;
}

const BoltAddress* BoltConnection_remote_endpoint(BoltConnection* connection)
{
    return connection->resolved_address;
}

const BoltAddress* BoltConnection_local_endpoint(BoltConnection* connection)
{
    return connection->local_address;
}

const char* BoltConnection_last_bookmark(BoltConnection* connection)
{
    return connection->protocol->last_bookmark(connection);
}

struct BoltValue* BoltConnection_field_names(BoltConnection* connection)
{
    return connection->protocol->field_names(connection);
}

struct BoltValue* BoltConnection_metadata(BoltConnection* connection)
{
    return connection->protocol->metadata(connection);
}

struct BoltValue* BoltConnection_failure(BoltConnection* connection)
{
    return connection->protocol->failure(connection);
}

BoltStatus* BoltConnection_status(BoltConnection* connection)
{
    return connection->status;
}
