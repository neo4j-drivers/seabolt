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

#include "bolt/config-impl.h"

#include "bolt/buffering.h"
#include "bolt/connections.h"
#include "bolt/logging.h"
#include "bolt/mem.h"

#include "protocol/protocol.h"
#include "protocol/v1.h"
#include "protocol/v2.h"
#include "protocol/v3.h"
#include "bolt/platform.h"
#include <assert.h>
#include <bolt/tls.h>

#define INITIAL_TX_BUFFER_SIZE 8192
#define INITIAL_RX_BUFFER_SIZE 8192

#define MAX_IPADDR_LEN 64

#define SOCKET(domain, type, protocol) socket(domain, type, protocol)
#define CONNECT(socket, address, address_size) connect(socket, address, address_size)
#define SHUTDOWN(socket, how) shutdown(socket, how)
#define TRANSMIT(socket, data, size, flags) (int)(send(socket, data, (size_t)(size), flags))
#define TRANSMIT_S(socket, data, size, flags) SSL_write(socket, data, size)
#define RECEIVE(socket, buffer, size, flags) (int)(recv(socket, buffer, (size_t)(size), flags))
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

#define TRY(code) { \
    int status = (code); \
    if (status != BOLT_SUCCESS) { \
        if (status == -1) { \
            _set_status(connection, BOLT_DEFUNCT, _last_error(connection)); \
        } else if (status == BOLT_STATUS_SET) { \
            return -1; \
        } else { \
            _set_status(connection, BOLT_DEFUNCT, status); \
        } \
        return status; \
    } \
}

#ifndef SHUT_RDWR
#define SHUT_RDWR SD_BOTH
#endif

enum BoltConnectionError _transform_error(int error_code)
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
    case EAGAIN:
        return BOLT_OUT_OF_PORTS;
    case ECONNREFUSED:
        return BOLT_CONNECTION_REFUSED;
    case ECONNRESET:
        return BOLT_CONNECTION_RESET;
    case EINTR:
        return BOLT_INTERRUPTED;
    case ENETUNREACH:
        return BOLT_NETWORK_UNREACHABLE;
    case ETIMEDOUT:
        return BOLT_TIMED_OUT;
    default:
        return BOLT_UNKNOWN_ERROR;
    }
#endif
}

int _last_error_code(struct BoltConnection* connection)
{
#if USE_WINSOCK
    return WSAGetLastError();
#else
    return errno;
#endif
}

enum BoltConnectionError _last_error(struct BoltConnection* connection)
{
    int error_code = _last_error_code(connection);
    BoltLog_error(connection->log, "socket error code: %d", error_code);
    return _transform_error(error_code);
}

void _set_status(struct BoltConnection* connection, enum BoltConnectionStatus status, enum BoltConnectionError error)
{
    enum BoltConnectionStatus old_status = connection->status;
    connection->status = status;
    connection->error = error;
    if (status!=old_status) {
        switch (connection->status) {
        case BOLT_DISCONNECTED:
            BoltLog_info(connection->log, "<DISCONNECTED>");
            break;
        case BOLT_CONNECTED:
            BoltLog_info(connection->log, "<CONNECTED>");
            break;
        case BOLT_READY:
            BoltLog_info(connection->log, "<READY>");
            break;
        case BOLT_FAILED:
            BoltLog_info(connection->log, "<FAILED>");
            break;
        case BOLT_DEFUNCT:
            BoltLog_info(connection->log, "<DEFUNCT>");
            break;
        }

        if (connection->status==BOLT_DEFUNCT || connection->status==BOLT_FAILED) {
            if (connection->on_error_cb!=NULL) {
                (*connection->on_error_cb)(connection, connection->on_error_cb_state);
            }
        }
    }
}

int _socket_configure(struct BoltConnection* connection)
{
    const int YES = 1, NO = 0;

    // Enable TCP_NODELAY
    TRY(SETSOCKETOPT(connection->socket, IPPROTO_TCP, TCP_NODELAY, &YES, sizeof(YES)));

    // Set keep-alive accordingly, default: TRUE
    if (connection->sock_opts==NULL || connection->sock_opts->keepalive) {
        TRY(SETSOCKETOPT(connection->socket, SOL_SOCKET, SO_KEEPALIVE, &YES, sizeof(YES)));
    }
    else {
        TRY(SETSOCKETOPT(connection->socket, SOL_SOCKET, SO_KEEPALIVE, &NO, sizeof(NO)));
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
    int recv_timeout_size = sizeof(struct timeval), int send_timeout_size = sizeof(struct timeval);
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

    TRY(SETSOCKETOPT(connection->socket, SOL_SOCKET, SO_RCVTIMEO, &recv_timeout, recv_timeout_size));
    TRY(SETSOCKETOPT(connection->socket, SOL_SOCKET, SO_SNDTIMEO, &send_timeout, send_timeout_size));

    return BOLT_SUCCESS;
}

int _socket_set_mode(struct BoltConnection* connection, int blocking)
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

int _open(struct BoltConnection* connection, enum BoltTransport transport, const struct sockaddr_storage* address)
{
    memset(&connection->metrics, 0, sizeof(connection->metrics));
    connection->transport = transport;
    switch (address->ss_family) {
    case AF_INET:
    case AF_INET6: {
        char host_string[NI_MAXHOST];
        char port_string[NI_MAXSERV];
        getnameinfo((const struct sockaddr*) (address), ADDR_SIZE(address),
                host_string, NI_MAXHOST, port_string, NI_MAXSERV, NI_NUMERICHOST | NI_NUMERICSERV);
        BoltLog_info(connection->log, "Opening %s connection to %s at port %s",
                address->ss_family==AF_INET ? "IPv4" : "IPv6", &host_string, &port_string);
        break;
    }
    default:
        BoltLog_error(connection->log, "Unsupported address family %d", address->ss_family);
        _set_status(connection, BOLT_DEFUNCT, BOLT_UNSUPPORTED);
        return BOLT_STATUS_SET;
    }
    connection->socket = (int) SOCKET(address->ss_family, SOCK_STREAM, IPPROTO_TCP);
    if (connection->socket==-1) {
        _set_status(connection, BOLT_DEFUNCT, _last_error(connection));
        return BOLT_STATUS_SET;
    }

    if (connection->sock_opts!=NULL && connection->sock_opts->connect_timeout>0) {
        // enable non-blocking mode
        TRY(_socket_set_mode(connection, 0));

        // initiate connection
        int status = CONNECT(connection->socket, (struct sockaddr*) (address), ADDR_SIZE(address));
        if (status==-1) {
            int error_code = _last_error_code(connection);
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
                _set_status(connection, BOLT_DEFUNCT, _transform_error(error_code));
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
                BoltLog_info(connection->log, "Connect timed out");
                _set_status(connection, BOLT_DEFUNCT, BOLT_TIMED_OUT);
                return BOLT_STATUS_SET;
            }
            case 1: {
                int so_error;
                int so_error_len = sizeof(int);

                TRY(GETSOCKETOPT(connection->socket, SOL_SOCKET, SO_ERROR, &so_error, &so_error_len));
                if (so_error!=0) {
                    _set_status(connection, BOLT_DEFUNCT, _transform_error(so_error));
                    return BOLT_STATUS_SET;
                }

                break;
            }
            default:
                //another error occurred
                _set_status(connection, BOLT_DEFUNCT, _last_error(connection));
                return BOLT_STATUS_SET;
            }
        }

        // revert to blocking mode
        TRY(_socket_set_mode(connection, 1));
    }
    else {
        TRY(CONNECT(connection->socket, (struct sockaddr*) (address), ADDR_SIZE(address)));
    }

    TRY(_socket_configure(connection));
    BoltUtil_get_time(&connection->metrics.time_opened);
    connection->tx_buffer = BoltBuffer_create(INITIAL_TX_BUFFER_SIZE);
    connection->rx_buffer = BoltBuffer_create(INITIAL_RX_BUFFER_SIZE);
    return BOLT_SUCCESS;
}

int _secure(struct BoltConnection* connection, struct BoltTrust* trust)
{
    int status = 1;
    // TODO: investigate ways to provide a greater resolution of TLS errors
    BoltLog_info(connection->log, "Securing socket");

    if (connection->ssl_context==NULL) {
        connection->ssl_context = create_ssl_ctx(trust, connection->address->host, connection->log);
        connection->owns_ssl_context = 1;
    }

    connection->ssl = SSL_new(connection->ssl_context);

    // Link to underlying socket
    if (status) {
        status = SSL_set_fd(connection->ssl, connection->socket);
    }

    // Enable SNI
    if (status) {
        status = SSL_set_tlsext_host_name(connection->ssl, connection->address->host);
    }

    status = SSL_connect(connection->ssl);
    if (status==1) {
        return 0;
    }

    _set_status(connection, BOLT_DEFUNCT, BOLT_TLS_ERROR);
    SSL_free(connection->ssl);
    if (connection->owns_ssl_context) {
        SSL_CTX_free(connection->ssl_context);
    }
    connection->owns_ssl_context = 0;
    connection->ssl_context = NULL;
    connection->ssl = NULL;

    return -1;
}

void _close(struct BoltConnection* connection)
{
    BoltLog_info(connection->log, "Closing connection");

    if (connection->protocol!=NULL) {
        int status = connection->protocol->goodbye(connection);
        if (status!=BOLT_SUCCESS) {
            BoltLog_warning(connection->log, "Unable to complete GOODBYE, status is %d", status);
        }

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

    switch (connection->transport) {
    case BOLT_SOCKET: {
        SHUTDOWN(connection->socket, SHUT_RDWR);
        CLOSE(connection->socket);
        break;
    }
    case BOLT_SECURE_SOCKET: {
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
    BoltUtil_get_time(&connection->metrics.time_closed);
    connection->socket = 0;
    _set_status(connection, BOLT_DISCONNECTED, BOLT_SUCCESS);
}

int _send(struct BoltConnection* connection, const char* data, int size)
{
    if (size==0) {
        return BOLT_SUCCESS;
    }
    int remaining = size;
    int total_sent = 0;
    while (total_sent<size) {
        int sent = 0;
        switch (connection->transport) {
        case BOLT_SOCKET: {
            sent = TRANSMIT(connection->socket, data+total_sent, remaining, 0);
            break;
        }
        case BOLT_SECURE_SOCKET: {
            sent = TRANSMIT_S(connection->ssl, data+total_sent, remaining, 0);
            break;
        }
        }
        if (sent>=0) {
            connection->metrics.bytes_sent += sent;
            total_sent += sent;
            remaining -= sent;
        }
        else {
            switch (connection->transport) {
            case BOLT_SOCKET:
                _set_status(connection, BOLT_DEFUNCT, _last_error(connection));
                BoltLog_error(connection->log, "Socket error %d on transmit", connection->error);
                break;
            case BOLT_SECURE_SOCKET:
                _set_status(connection, BOLT_DEFUNCT, SSL_get_error(connection->ssl, sent));
                BoltLog_error(connection->log, "SSL error %d on transmit", connection->error);
                break;
            }
            return BOLT_TRANSPORT_UNSUPPORTED;
        }
    }
    BoltLog_info(connection->log, "(Sent %d of %d bytes)", total_sent, size);
    return BOLT_SUCCESS;
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
int _receive(struct BoltConnection* connection, char* buffer, int min_size, int max_size, int* received)
{
    if (min_size==0) {
        return BOLT_SUCCESS;
    }
    int max_remaining = max_size;
    int total_received = 0;
    while (total_received<min_size) {
        int single_received = 0;
        switch (connection->transport) {
        case BOLT_SOCKET:
            single_received = RECEIVE(connection->socket, buffer+total_received, max_remaining, 0);
            break;
        case BOLT_SECURE_SOCKET:
            single_received = RECEIVE_S(connection->ssl, buffer+total_received, max_remaining, 0);
            break;
        }
        if (single_received>0) {
            connection->metrics.bytes_received += single_received;
            total_received += single_received;
            max_remaining -= single_received;
        }
        else if (single_received==0) {
            BoltLog_info(connection->log, "Detected end of transmission");
            _set_status(connection, BOLT_DISCONNECTED, BOLT_END_OF_TRANSMISSION);
            return BOLT_STATUS_SET;
        }
        else {
            switch (connection->transport) {
            case BOLT_SOCKET:
                _set_status(connection, BOLT_DEFUNCT, _last_error(connection));
                BoltLog_error(connection->log, "Socket error %d on receive", connection->error);
                break;
            case BOLT_SECURE_SOCKET:
                _set_status(connection, BOLT_DEFUNCT, SSL_get_error(connection->ssl, single_received));
                BoltLog_error(connection->log, "SSL error %d on receive", connection->error);
                break;
            }
            return BOLT_STATUS_SET;
        }
    }
    if (min_size==max_size) {
        BoltLog_info(connection->log, "(Received %d of %d bytes)", total_received, max_size);
    }
    else {
        BoltLog_info(connection->log, "(Received %d of %d..%d bytes)", total_received, min_size, max_size);
    }
    *received = total_received;
    return BOLT_SUCCESS;
}

int handshake_b(struct BoltConnection* connection, int32_t _1, int32_t _2, int32_t _3, int32_t _4)
{
    BoltLog_info(connection->log, "Performing handshake");
    char handshake[20];
    memcpy(&handshake[0x00], "\x60\x60\xB0\x17", 4);
    memcpy_be(&handshake[0x04], &_1, 4);
    memcpy_be(&handshake[0x08], &_2, 4);
    memcpy_be(&handshake[0x0C], &_3, 4);
    memcpy_be(&handshake[0x10], &_4, 4);
    TRY(_send(connection, &handshake[0], 20));
    int received = 0;
    TRY(_receive(connection, &handshake[0], 4, 4, &received));
    memcpy_be(&connection->protocol_version, &handshake[0], 4);
    BoltLog_info(connection->log, "<SET protocol_version=%d>", connection->protocol_version);
    switch (connection->protocol_version) {
    case 1:
        connection->protocol = BoltProtocolV1_create_protocol();
        return BOLT_SUCCESS;
    case 2:
        connection->protocol = BoltProtocolV2_create_protocol();
        return BOLT_SUCCESS;
    case 3:
        connection->protocol = BoltProtocolV3_create_protocol();
        return BOLT_SUCCESS;
    default:
        _close(connection);
        return BOLT_PROTOCOL_UNSUPPORTED;
    }
}

struct BoltConnection* BoltConnection_create()
{
    const size_t size = sizeof(struct BoltConnection);
    struct BoltConnection* connection = BoltMem_allocate(size);
    memset(connection, 0, size);
    return connection;
}

void BoltConnection_destroy(struct BoltConnection* connection)
{
    BoltMem_deallocate(connection, sizeof(struct BoltConnection));
}

int
BoltConnection_open(struct BoltConnection* connection, enum BoltTransport transport, struct BoltAddress* address,
        struct BoltTrust* trust, struct BoltLog* log, struct BoltSocketOptions* sock_opts)
{
    if (connection->status!=BOLT_DISCONNECTED) {
        BoltConnection_close(connection);
    }
    connection->log = log;
    // Store connection info
    connection->address = BoltAddress_create(address->host, address->port);
    // Store socket options
    connection->sock_opts = sock_opts;
    for (int i = 0; i<address->n_resolved_hosts; i++) {
        const int opened = _open(connection, transport, &address->resolved_hosts[i]);
        if (opened==BOLT_SUCCESS) {
            if (connection->transport==BOLT_SECURE_SOCKET) {
                const int secured = _secure(connection, trust);
                if (secured==0) {
                    TRY(handshake_b(connection, 3, 2, 1, 0));
                }
                else {
                    return connection->error;
                }
            }
            else {
                TRY(handshake_b(connection, 3, 2, 1, 0));
            }

            char resolved_host[MAX_IPADDR_LEN], resolved_port[6];
            BoltAddress_copy_resolved_host(address, i, resolved_host, MAX_IPADDR_LEN);
            sprintf(resolved_port, "%d", address->resolved_port);
            connection->resolved_address = BoltAddress_create(resolved_host, resolved_port);

            _set_status(connection, BOLT_CONNECTED, BOLT_SUCCESS);
            break;
        }
    }
    if (connection->status==BOLT_DISCONNECTED) {
        _set_status(connection, BOLT_DEFUNCT, BOLT_NO_VALID_ADDRESS);
        return -1;
    }
    return connection->status==BOLT_READY ? BOLT_SUCCESS : connection->error;
}

void BoltConnection_close(struct BoltConnection* connection)
{
    if (connection->status!=BOLT_DISCONNECTED) {
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
}

int BoltConnection_send(struct BoltConnection* connection)
{
    int size = BoltBuffer_unloadable(connection->tx_buffer);
    TRY(_send(connection, BoltBuffer_unload_pointer(connection->tx_buffer, size), size));
    BoltBuffer_compact(connection->tx_buffer);
    return 0;
}

int BoltConnection_receive(struct BoltConnection* connection, char* buffer, int size)
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
                    &received));
            // adjust the buffer extent based on the actual amount of data received
            connection->rx_buffer->extent = connection->rx_buffer->extent-max_size+received;
            delta -= received;
        }
    }
    BoltBuffer_unload(connection->rx_buffer, buffer, size);
    return BOLT_SUCCESS;
}

int BoltConnection_fetch(struct BoltConnection* connection, bolt_request request)
{
    const int fetched = connection->protocol->fetch(connection, request);
    if (fetched==FETCH_SUMMARY) {
        if (connection->protocol->is_success_summary(connection)) {
            _set_status(connection, BOLT_READY, BOLT_SUCCESS);
        }
        else if (connection->protocol->is_ignored_summary(connection)) {
            // we may need to update status based on an earlier reported FAILURE
            // which our consumer did not care its result
            if (connection->protocol->failure(connection)!=NULL) {
                _set_status(connection, BOLT_FAILED, BOLT_SERVER_FAILURE);
            }
        }
        else if (connection->protocol->is_failure_summary(connection)) {
            _set_status(connection, BOLT_FAILED, BOLT_SERVER_FAILURE);
        }
        else {
            BoltLog_error(connection->log, "Protocol violation (received summary code %d)",
                    connection->protocol->last_data_type(connection));
            _set_status(connection, BOLT_DEFUNCT, BOLT_PROTOCOL_VIOLATION);
            return FETCH_ERROR;
        }
    }
    return fetched;
}

int BoltConnection_fetch_summary(struct BoltConnection* connection, bolt_request request)
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

struct BoltValue* BoltConnection_field_values(struct BoltConnection* connection)
{
    return connection->protocol->field_values(connection);
}

int BoltConnection_summary_success(struct BoltConnection* connection)
{
    return connection->protocol->is_success_summary(connection);
}

int BoltConnection_summary_failure(struct BoltConnection* connection)
{
    return connection->protocol->is_failure_summary(connection);
}

int
BoltConnection_init(struct BoltConnection* connection, const char* user_agent, const struct BoltValue* auth_token)
{
    BoltLog_info(connection->log, "Initialising connection");
    switch (connection->protocol_version) {
    case 1:
    case 2:
    case 3: {
        int code = connection->protocol->init(connection, user_agent, auth_token);
        switch (code) {
        case BOLT_V1_SUCCESS:
            _set_status(connection, BOLT_READY, BOLT_SUCCESS);
            return 0;
        case BOLT_V1_FAILURE:
            _set_status(connection, BOLT_DEFUNCT, BOLT_PERMISSION_DENIED);
            return -1;
        default:
            BoltLog_error(connection->log, "Protocol violation (received summary code %d)", code);
            _set_status(connection, BOLT_DEFUNCT, BOLT_PROTOCOL_VIOLATION);
            return -1;
        }
    }
    default:
        _set_status(connection, BOLT_DEFUNCT, BOLT_PROTOCOL_UNSUPPORTED);
        return -1;
    }
}

int BoltConnection_clear_begin(struct BoltConnection* connection)
{
    TRY(connection->protocol->clear_begin_tx(connection));
    return BOLT_SUCCESS;
}

int BoltConnection_set_begin_bookmarks(struct BoltConnection* connection, struct BoltValue* bookmark_list)
{
    TRY(connection->protocol->set_begin_tx_bookmark(connection, bookmark_list));
    return BOLT_SUCCESS;
}

int BoltConnection_set_begin_tx_timeout(struct BoltConnection* connection, int64_t timeout)
{
    TRY(connection->protocol->set_begin_tx_timeout(connection, timeout));
    return BOLT_SUCCESS;
}

int BoltConnection_set_begin_tx_metadata(struct BoltConnection* connection, struct BoltValue* metadata)
{
    TRY(connection->protocol->set_begin_tx_metadata(connection, metadata));
    return BOLT_SUCCESS;
}

int BoltConnection_load_begin_request(struct BoltConnection* connection)
{
    TRY(connection->protocol->load_begin_tx(connection));
    return BOLT_SUCCESS;
}

int BoltConnection_load_commit_request(struct BoltConnection* connection)
{
    TRY(connection->protocol->load_commit_tx(connection));
    return BOLT_SUCCESS;
}

int BoltConnection_load_rollback_request(struct BoltConnection* connection)
{
    TRY(connection->protocol->load_rollback_tx(connection));
    return BOLT_SUCCESS;
}

int BoltConnection_clear_run(struct BoltConnection* connection)
{
    TRY(connection->protocol->clear_run(connection));
    return BOLT_SUCCESS;
}

int
BoltConnection_set_run_cypher(struct BoltConnection* connection, const char* cypher, const size_t cypher_size,
        int32_t n_parameter)
{
    TRY(connection->protocol->set_run_cypher(connection, cypher, cypher_size, n_parameter));
    return BOLT_SUCCESS;
}

struct BoltValue*
BoltConnection_set_run_cypher_parameter(struct BoltConnection* connection, int32_t index, const char* name,
        size_t name_size)
{
    return connection->protocol->set_run_cypher_parameter(connection, index, name, name_size);
}

int BoltConnection_set_run_bookmarks(struct BoltConnection* connection, struct BoltValue* bookmark_list)
{
    TRY(connection->protocol->set_run_bookmark(connection, bookmark_list));
    return BOLT_SUCCESS;
}

int BoltConnection_set_run_tx_timeout(struct BoltConnection* connection, int64_t timeout)
{
    TRY(connection->protocol->set_run_tx_timeout(connection, timeout));
    return BOLT_SUCCESS;
}

int BoltConnection_set_run_tx_metadata(struct BoltConnection* connection, struct BoltValue* metadata)
{
    TRY(connection->protocol->set_run_tx_metadata(connection, metadata));
    return BOLT_SUCCESS;
}

int BoltConnection_load_run_request(struct BoltConnection* connection)
{
    TRY(connection->protocol->load_run(connection));
    return BOLT_SUCCESS;
}

int BoltConnection_load_discard_request(struct BoltConnection* connection, int32_t n)
{
    TRY(connection->protocol->load_discard(connection, n));
    return BOLT_SUCCESS;
}

int BoltConnection_load_pull_request(struct BoltConnection* connection, int32_t n)
{
    TRY(connection->protocol->load_pull(connection, n));
    return BOLT_SUCCESS;
}

int BoltConnection_load_reset_request(struct BoltConnection* connection)
{
    TRY(connection->protocol->load_reset(connection));
    return BOLT_SUCCESS;
}

bolt_request BoltConnection_last_request(struct BoltConnection* connection)
{
    return connection->protocol->last_request(connection);
}

char* BoltConnection_last_bookmark(struct BoltConnection* connection)
{
    return connection->protocol->last_bookmark(connection);
}

struct BoltValue* BoltConnection_field_names(struct BoltConnection* connection)
{
    return connection->protocol->field_names(connection);
}

struct BoltValue* BoltConnection_metadata(struct BoltConnection* connection)
{
    return connection->protocol->metadata(connection);
}

struct BoltValue* BoltConnection_failure(struct BoltConnection* connection)
{
    return connection->protocol->failure(connection);
}

char* BoltConnection_server(struct BoltConnection* connection)
{
    return connection->protocol->server(connection);
}
