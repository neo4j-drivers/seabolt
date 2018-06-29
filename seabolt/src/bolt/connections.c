/*
 * Copyright (c) 2002-2018 "Neo Technology,"
 * Network Engine for Objects in Lund AB [http://neotechnology.com]
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

#include "protocol/v1.h"
#include "bolt/platform.h"
#include <assert.h>

#define INITIAL_TX_BUFFER_SIZE 8192
#define INITIAL_RX_BUFFER_SIZE 8192

#define SOCKET(domain, type, protocol) socket(domain, type, protocol)
#define CONNECT(socket, address, address_size) connect(socket, address, address_size)
#define SHUTDOWN(socket, how) shutdown(socket, how)
#define TRANSMIT(socket, data, size, flags) (int)(send(socket, data, (size_t)(size), flags))
#define TRANSMIT_S(socket, data, size, flags) SSL_write(socket, data, size)
#define RECEIVE(socket, buffer, size, flags) (int)(recv(socket, buffer, (size_t)(size), flags))
#define RECEIVE_S(socket, buffer, size, flags) SSL_read(socket, buffer, size)
#define ADDR_SIZE(address) address->ss_family == AF_INET ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6)
#if USE_WINSOCK
#define SETSOCKETOPT(socket, level, optname, optval, optlen) setsockopt(socket, level, optname, (const char *)(optval), optlen)
#define CLOSE(socket) closesocket(socket)
#else
#define SETSOCKETOPT(socket, level, optname, optval, optlen) setsockopt(socket, level, optname, (const void *)optval, optlen)
#define CLOSE(socket) close(socket)
#endif

#define TRY(code) { int status = (code); if (status == -1) { _set_status(connection, BOLT_DEFUNCT, _last_error()); return status; } }

#ifndef SHUT_RDWR
#define SHUT_RDWR SD_BOTH
#endif

enum BoltConnectionError _last_error()
{
#if USE_WINSOCK
    int error_code = WSAGetLastError();
    BoltLog_error("bolt: Winsock error code: %d", error_code);
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
    int error_code = errno;
    BoltLog_error("bolt: socket error code: %d", error_code);
    switch (error_code)
    {
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

void _set_status(struct BoltConnection* connection, enum BoltConnectionStatus status, enum BoltConnectionError error)
{
    enum BoltConnectionStatus old_status = connection->status;
    connection->status = status;
    connection->error = error;
    if (status!=old_status) {
        switch (connection->status) {
        case BOLT_DISCONNECTED:
            BoltLog_info("bolt: <DISCONNECTED>");
            break;
        case BOLT_CONNECTED:
            BoltLog_info("bolt: <CONNECTED>");
            break;
        case BOLT_READY:
            BoltLog_info("bolt: <READY>");
            break;
        case BOLT_FAILED:
            BoltLog_info("bolt: <FAILED>");
            break;
        case BOLT_DEFUNCT:
            BoltLog_info("bolt: <DEFUNCT>");
            break;
        }
    }
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
        BoltLog_info("bolt: Opening %s connection to %s at port %s",
                address->ss_family==AF_INET ? "IPv4" : "IPv6", &host_string, &port_string);
        break;
    }
    default:
        BoltLog_error("bolt: Unsupported address family %d", address->ss_family);
        _set_status(connection, BOLT_DEFUNCT, BOLT_UNSUPPORTED);
        return -1;
    }
    connection->socket = (int)SOCKET(address->ss_family, SOCK_STREAM, IPPROTO_TCP);
    if (connection->socket==-1) {
        _set_status(connection, BOLT_DEFUNCT, _last_error());
        return -1;
    }
    const int YES = 1;
    // TODO: socket options in Windows
    TRY(SETSOCKETOPT(connection->socket, SOL_SOCKET, SO_KEEPALIVE, &YES, sizeof(YES)));
    TRY(SETSOCKETOPT(connection->socket, IPPROTO_TCP, TCP_NODELAY, &YES, sizeof(YES)));
    TRY(CONNECT(connection->socket, (struct sockaddr*) (address), ADDR_SIZE(address)));
    BoltUtil_get_time(&connection->metrics.time_opened);
    connection->tx_buffer = BoltBuffer_create(INITIAL_TX_BUFFER_SIZE);
    connection->rx_buffer = BoltBuffer_create(INITIAL_RX_BUFFER_SIZE);
    return 0;
}

int _secure(struct BoltConnection* connection)
{
    // TODO: investigate ways to provide a greater resolution of TLS errors
    BoltLog_info("bolt: Securing socket");
    const SSL_METHOD* ctx_init_method = NULL;
#if OPENSSL_VERSION_NUMBER<0x10100000L
    ctx_init_method = TLSv1_2_client_method();
#else
    ctx_init_method = TLS_client_method();
#endif
    SSL_library_init();
    SSL_load_error_strings();
    OpenSSL_add_all_algorithms();
    connection->ssl_context = SSL_CTX_new(ctx_init_method);
    if (connection->ssl_context==NULL) {
        _set_status(connection, BOLT_DEFUNCT, BOLT_TLS_ERROR);
        return -1;
    }
    if (SSL_CTX_set_default_verify_paths(connection->ssl_context)!=1) {
        _set_status(connection, BOLT_DEFUNCT, BOLT_TLS_ERROR);
        return -1;
    }
    connection->ssl = SSL_new(connection->ssl_context);
    int linked_socket = SSL_set_fd(connection->ssl, connection->socket);
    if (linked_socket!=1) {
        _set_status(connection, BOLT_DEFUNCT, BOLT_TLS_ERROR);
        return -1;
    }
    int connected = SSL_connect(connection->ssl);
    if (connected!=1) {
        _set_status(connection, BOLT_DEFUNCT, BOLT_TLS_ERROR);
        return -1;
    }
    return 0;
}

void _close(struct BoltConnection* connection)
{
    BoltLog_info("bolt: Closing connection");
    switch (connection->protocol_version) {
    case 1:
        BoltProtocolV1_destroy_state(connection->protocol_state);
        connection->protocol_state = NULL;
        break;
    default:
        break;
    }
    connection->protocol_version = 0;
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
        if (connection->ssl_context!=NULL) {
            SSL_CTX_free(connection->ssl_context);
            connection->ssl_context = NULL;
        }
        SHUTDOWN(connection->socket, SHUT_RDWR);
        CLOSE(connection->socket);
        break;
    }
    }
    BoltUtil_get_time(&connection->metrics.time_closed);
    connection->socket = 0;
    _set_status(connection, BOLT_DISCONNECTED, BOLT_NO_ERROR);
}

int _send(struct BoltConnection* connection, const char* data, int size)
{
    if (size==0) {
        return 0;
    }
//    printf("bolt: (Sending");
//    for (int i = 0; i < size; i++) { printf(" %c%c", hex1(data, i), hex0(data, i)); }
//    printf(")\n");
    int remaining = size;
    int total_sent = 0;
    while (total_sent<size) {
        int sent = 0;
        switch (connection->transport) {
        case BOLT_SOCKET: {
            sent = TRANSMIT(connection->socket, data, size, 0);
            break;
        }
        case BOLT_SECURE_SOCKET: {
            sent = TRANSMIT_S(connection->ssl, data, size, 0);
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
                _set_status(connection, BOLT_DEFUNCT, _last_error());
                BoltLog_error("bolt: Socket error %d on transmit", connection->error);
                break;
            case BOLT_SECURE_SOCKET:
                _set_status(connection, BOLT_DEFUNCT, SSL_get_error(connection->ssl, sent));
                BoltLog_error("bolt: SSL error %d on transmit", connection->error);
                break;
            }
            return -1;
        }
    }
    BoltLog_info("bolt: (Sent %d of %d bytes)", total_sent, size);
    return total_sent;
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
int _receive(struct BoltConnection* connection, char* buffer, int min_size, int max_size)
{
    if (min_size==0) {
        return 0;
    }
    int max_remaining = max_size;
    int total_received = 0;
    while (total_received<min_size) {
        int received = 0;
        switch (connection->transport) {
        case BOLT_SOCKET:
            received = RECEIVE(connection->socket, buffer, max_remaining, 0);
            break;
        case BOLT_SECURE_SOCKET:
            received = RECEIVE_S(connection->ssl, buffer, max_remaining, 0);
            break;
        }
        if (received>0) {
            connection->metrics.bytes_received += received;
            total_received += received;
            max_remaining -= received;
        }
        else if (received==0) {
            BoltLog_info("bolt: Detected end of transmission");
            _set_status(connection, BOLT_DISCONNECTED, BOLT_END_OF_TRANSMISSION);
            return -1;
        }
        else {
            switch (connection->transport) {
            case BOLT_SOCKET:
                _set_status(connection, BOLT_DEFUNCT, _last_error());
                BoltLog_error("bolt: Socket error %d on receive", connection->error);
                break;
            case BOLT_SECURE_SOCKET:
                _set_status(connection, BOLT_DEFUNCT, SSL_get_error(connection->ssl, received));
                BoltLog_error("bolt: SSL error %d on receive", connection->error);
                break;
            }
            return -1;
        }
    }
    if (min_size==max_size) {
        BoltLog_info("bolt: (Received %d of %d bytes)", total_received, max_size);
    }
    else {
        BoltLog_info("bolt: (Received %d of %d..%d bytes)", total_received, min_size, max_size);
    }
//    printf("bolt: (Received");
//    for (int i = 0; i < total_received; i++) { printf(" %c%c", hex1(buffer, i), hex0(buffer, i)); }
//    printf(")\n");
    return total_received;
}

int handshake_b(struct BoltConnection* connection, int32_t _1, int32_t _2, int32_t _3, int32_t _4)
{
    BoltLog_info("bolt: Performing handshake");
    char handshake[20];
    memcpy(&handshake[0x00], "\x60\x60\xB0\x17", 4);
    memcpy_be(&handshake[0x04], &_1, 4);
    memcpy_be(&handshake[0x08], &_2, 4);
    memcpy_be(&handshake[0x0C], &_3, 4);
    memcpy_be(&handshake[0x10], &_4, 4);
    TRY(_send(connection, &handshake[0], 20));
    TRY(_receive(connection, &handshake[0], 4, 4));
    memcpy_be(&connection->protocol_version, &handshake[0], 4);
    BoltLog_info("bolt: <SET protocol_version=%d>", connection->protocol_version);
    switch (connection->protocol_version) {
    case 1:
        connection->protocol_state = BoltProtocolV1_create_state();
        return 0;
    default:
        _close(connection);
        _set_status(connection, BOLT_DEFUNCT, BOLT_UNSUPPORTED);
        return -1;
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

int BoltConnection_open(struct BoltConnection* connection, enum BoltTransport transport, struct BoltAddress* address)
{
    if (connection->status!=BOLT_DISCONNECTED) {
        BoltConnection_close(connection);
    }
    for (int i = 0; i<address->n_resolved_hosts; i++) {
        const int opened = _open(connection, transport, &address->resolved_hosts[i]);
        if (opened==0) {
            if (connection->transport==BOLT_SECURE_SOCKET) {
                const int secured = _secure(connection);
                if (secured==0) {
                    handshake_b(connection, 1, 0, 0, 0);
                }
            }
            else {
                handshake_b(connection, 1, 0, 0, 0);
            }
            _set_status(connection, BOLT_CONNECTED, BOLT_NO_ERROR);
            break;
        }
    }
    if (connection->status==BOLT_DISCONNECTED) {
        _set_status(connection, BOLT_DEFUNCT, BOLT_NO_VALID_ADDRESS);
        return -1;
    }
    return 0;
}

void BoltConnection_close(struct BoltConnection* connection)
{
    if (connection->rx_buffer!=NULL) {
        BoltBuffer_destroy(connection->rx_buffer);
        connection->rx_buffer = NULL;
    }
    if (connection->tx_buffer!=NULL) {
        BoltBuffer_destroy(connection->tx_buffer);
        connection->tx_buffer = NULL;
    }
    if (connection->status!=BOLT_DISCONNECTED) {
        _close(connection);
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
            int received = _receive(connection, BoltBuffer_load_pointer(connection->rx_buffer, max_size), delta,
                    max_size);
            if (received==-1) {
                return -1;
            }
            // adjust the buffer extent based on the actual amount of data received
            connection->rx_buffer->extent = connection->rx_buffer->extent-max_size+received;
            delta -= received;
        }
    }
    BoltBuffer_unload(connection->rx_buffer, buffer, size);
    return size;
}

int BoltConnection_fetch(struct BoltConnection* connection, bolt_request_t request)
{
    switch (connection->protocol_version) {
    case 1: {
        const int fetched = BoltProtocolV1_fetch(connection, request);
        if (fetched==0) {
            // Summary received
            struct BoltProtocolV1State* state = BoltProtocolV1_state(connection);
            switch (state->data_type) {
            case BOLT_V1_SUCCESS:
                _set_status(connection, BOLT_READY, BOLT_NO_ERROR);
                return 0;
            case BOLT_V1_IGNORED:
                // we may need to update status based on an earlier reported FAILURE
                // which our consumer did not care its result
                if (state->failure_data!=NULL) {
                    _set_status(connection, BOLT_FAILED, BOLT_SERVER_FAILURE);
                }

                return 0;
            case BOLT_V1_FAILURE:
                _set_status(connection, BOLT_FAILED, BOLT_SERVER_FAILURE);
                return 0;
            default:
                BoltLog_error("bolt: Protocol violation (received summary code %d)", state->data_type);
                _set_status(connection, BOLT_DEFUNCT, BOLT_PROTOCOL_VIOLATION);
                return -1;
            }
        }
        return fetched;
    }
    default: {
        // TODO
        return -1;
    }
    }
}

int BoltConnection_fetch_summary(struct BoltConnection* connection, bolt_request_t request)
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

struct BoltValue* BoltConnection_record_fields(struct BoltConnection* connection)
{
    switch (connection->protocol_version) {
    case 1: {
        struct BoltProtocolV1State* state = BoltProtocolV1_state(connection);
        switch (state->data_type) {
        case BOLT_V1_RECORD:
            switch (BoltValue_type(state->data)) {
            case BOLT_LIST: {
                struct BoltValue* values = BoltList_value(state->data, 0);
                return values;
            }
            default:
                return NULL;
            }
        default:
            return NULL;
        }
    }
    default:
        return NULL;
    }
}

int BoltConnection_summary_success(struct BoltConnection* connection)
{
    switch (connection->protocol_version) {
    case 1: {
        struct BoltProtocolV1State* state = BoltProtocolV1_state(connection);
        return state->data_type==BOLT_V1_SUCCESS;
    }
    default:
        return -1;
    }
}

int BoltConnection_summary_failure(struct BoltConnection* connection)
{
    switch (connection->protocol_version) {
    case 1: {
        struct BoltProtocolV1State* state = BoltProtocolV1_state(connection);
        return state->data_type==BOLT_V1_FAILURE;
    }
    default:
        return -1;
    }
}

int BoltConnection_init(struct BoltConnection* connection, const char* user_agent, const struct BoltValue* auth_token)
{
    BoltLog_info("bolt: Initialising connection");
    switch (connection->protocol_version) {
    case 1: {
        int code = BoltProtocolV1_init(connection, user_agent, auth_token);
        switch (code) {
        case BOLT_V1_SUCCESS:
            _set_status(connection, BOLT_READY, BOLT_NO_ERROR);
            return 0;
        case BOLT_V1_FAILURE:
            _set_status(connection, BOLT_DEFUNCT, BOLT_PERMISSION_DENIED);
            return -1;
        default:
            BoltLog_error("bolt: Protocol violation (received summary code %d)", code);
            _set_status(connection, BOLT_DEFUNCT, BOLT_PROTOCOL_VIOLATION);
            return -1;
        }
    }
    default:
        _set_status(connection, BOLT_DEFUNCT, BOLT_UNSUPPORTED);
        return -1;
    }
}

int BoltConnection_reset(struct BoltConnection* connection)
{
    BoltLog_info("bolt: Resetting connection");
    switch (connection->protocol_version) {
    case 1: {
        int code = BoltProtocolV1_reset(connection);
        switch (code) {
        case BOLT_V1_SUCCESS:
            _set_status(connection, BOLT_READY, BOLT_NO_ERROR);
            return 0;
        default:
            BoltLog_error("bolt: Connection failed to reset");
            _set_status(connection, BOLT_DEFUNCT, BOLT_UNKNOWN_ERROR);
            return -1;
        }
    }
    default:
        _set_status(connection, BOLT_DEFUNCT, BOLT_UNSUPPORTED);
        return -1;
    }
}

int
BoltConnection_cypher(struct BoltConnection* connection, const char* cypher, size_t cypher_size, int32_t n_parameters)
{
    switch (connection->protocol_version) {
    case 1:
        switch (BoltProtocolV1_set_cypher_template(connection, cypher, cypher_size)) {
        case 0:
            return BoltProtocolV1_set_n_cypher_parameters(connection, n_parameters);
        default:
            return -1;
        }
    default:
        return -1;
    }
}

struct BoltValue*
BoltConnection_cypher_parameter(struct BoltConnection* connection, int32_t index, const char* key, size_t key_size)
{
    switch (connection->protocol_version) {
    case 1:
        switch (BoltProtocolV1_set_cypher_parameter_key(connection, index, key, key_size)) {
        case 0:
            return BoltProtocolV1_cypher_parameter_value(connection, index);
        default:
            return NULL;
        }
    default:
        return NULL;
    }
}

int BoltConnection_ack_failure(struct BoltConnection* connection)
{
    assert(BoltConnection_failure(connection)!=NULL);

    switch (connection->protocol_version) {
    case 1:
        BoltProtocolV1_load_ack_failure(connection);
        BoltProtocolV1_clear_failure(connection);

        return 0;
    default:
        return -1;
    }
}

int BoltConnection_load_bookmark(struct BoltConnection* connection, const char* bookmark)
{
    switch (connection->protocol_version) {
    case 1:
        return BoltProtocolV1_load_bookmark(connection, bookmark);
    default:
        return -1;
    }
}

int BoltConnection_load_begin_request(struct BoltConnection* connection)
{
    switch (connection->protocol_version) {
    case 1:
        return BoltProtocolV1_load_begin_request(connection);
    default:
        return -1;
    }
}

int BoltConnection_load_commit_request(struct BoltConnection* connection)
{
    switch (connection->protocol_version) {
    case 1:
        return BoltProtocolV1_load_commit_request(connection);
    default:
        return -1;
    }
}

int BoltConnection_load_rollback_request(struct BoltConnection* connection)
{
    switch (connection->protocol_version) {
    case 1:
        return BoltProtocolV1_load_rollback_request(connection);
    default:
        return -1;
    }
}

int BoltConnection_load_run_request(struct BoltConnection* connection)
{
    switch (connection->protocol_version) {
    case 1:
        return BoltProtocolV1_load_run_request(connection);
    default:
        return -1;
    }
}

int BoltConnection_load_discard_request(struct BoltConnection* connection, int32_t n)
{
    switch (connection->protocol_version) {
    case 1:
        if (n>=0) {
            return -1;
        }
        else {
            struct BoltProtocolV1State* state = BoltProtocolV1_state(connection);
            BoltProtocolV1_load_message(connection, state->discard_request);
            return 0;
        }
    default:
        return -1;
    }
}

int BoltConnection_load_pull_request(struct BoltConnection* connection, int32_t n)
{
    switch (connection->protocol_version) {
    case 1:
        return BoltProtocolV1_load_pull_request(connection, n);
    default:
        return -1;
    }
}

bolt_request_t BoltConnection_last_request(struct BoltConnection* connection)
{
    switch (connection->protocol_version) {
    case 1: {
        struct BoltProtocolV1State* state = BoltProtocolV1_state(connection);
        if (state==NULL) {
            return 0;
        }
        return state->next_request_id-1;
    }
    default: {
        // TODO
        return 0;
    }
    }
}

char * BoltConnection_last_bookmark(struct BoltConnection* connection)
{
    switch (connection->protocol_version) {
    case 1: {
        struct BoltProtocolV1State* state = BoltProtocolV1_state(connection);
        if (state==NULL) {
            return 0;
        }
        return state->last_bookmark;
    }
    default: {
        // TODO
        return 0;
    }
    }
}

struct BoltValue* BoltConnection_metadata_fields(struct BoltConnection* connection)
{
    switch (connection->protocol_version) {
    case 1:
        return BoltProtocolV1_result_fields(connection);
    default:
        return NULL;
    }
}

struct BoltValue* BoltConnection_failure(struct BoltConnection* connection)
{
    switch (connection->protocol_version) {
    case 1: {
        struct BoltProtocolV1State* state = BoltProtocolV1_state(connection);
        if (state==NULL) {
            return NULL;
        }

        return state->failure_data;
    }
    default:
        return NULL;
    }
}
