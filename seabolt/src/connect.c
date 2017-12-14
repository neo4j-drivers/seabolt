/*
 * Copyright (c) 2002-2017 "Neo Technology,"
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

#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <connect.h>
#include <protocol_v1.h>
#include <buffer.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <bolt.h>


#define INITIAL_TX_BUFFER_SIZE 8192
#define INITIAL_RX_BUFFER_SIZE 8192

#define char_to_int16be(array) (header[0] << 8) | (header[1]);

#define SOCKET(domain, type, protocol) socket(domain, type, protocol)
#define CONNECT(socket, address, address_size) connect(socket, address, address_size)
#define SHUTDOWN(socket, how) shutdown(socket, how)
#define TRANSMIT(socket, data, size, flags) (int)(send(socket, data, (size_t)(size), flags))
#define TRANSMIT_S(socket, data, size, flags) SSL_write(socket, data, size)
#define RECEIVE(socket, buffer, size, flags) (int)(recv(socket, buffer, (size_t)(size), flags))
#define RECEIVE_S(socket, buffer, size, flags) SSL_read(socket, buffer, size)


static const char* DEFAULT_USER_AGENT = "seabolt/1.0.0a";


struct BoltConnection* _create(enum BoltTransport transport)
{
    struct BoltConnection* connection = BoltMem_allocate(sizeof(struct BoltConnection));
    memset(connection, 0, sizeof(struct BoltConnection));
    connection->transport = transport;
    connection->user_agent = DEFAULT_USER_AGENT;
    connection->tx_buffer_1 = BoltBuffer_create(INITIAL_TX_BUFFER_SIZE);
    connection->tx_buffer_0 = BoltBuffer_create(INITIAL_TX_BUFFER_SIZE);
    connection->rx_buffer_0 = BoltBuffer_create(INITIAL_RX_BUFFER_SIZE);
    connection->rx_buffer_1 = BoltBuffer_create(INITIAL_RX_BUFFER_SIZE);
    connection->current = BoltValue_create();
}

void _destroy(struct BoltConnection* connection)
{
    BoltValue_destroy(connection->current);
    BoltBuffer_destroy(connection->rx_buffer_1);
    BoltBuffer_destroy(connection->rx_buffer_0);
    BoltBuffer_destroy(connection->tx_buffer_0);
    BoltBuffer_destroy(connection->tx_buffer_1);
    BoltMem_deallocate(connection, sizeof(struct BoltConnection));
}

void _set_status(struct BoltConnection* connection, enum BoltConnectionStatus status, int error)
{
    connection->status = status;
    connection->error = error;
    switch (connection->status)
    {
        case BOLT_DISCONNECTED:
            BoltLog_info("bolt: Disconnected");
            break;
        case BOLT_CONNECTED:
            BoltLog_info("bolt: Connected to %s", &connection->address_string);
            break;
        case BOLT_READY:
            BoltLog_info("bolt: Ready");
            break;
        case BOLT_FAILED:
            BoltLog_info("bolt: FAILURE");
            break;
        case BOLT_DEFUNCT:
            BoltLog_info("bolt: DEFUNCT");
            break;
    }
}

int _open_b(struct BoltConnection* connection, const struct addrinfo* address)
{
    inet_ntop(address->ai_family, &((struct sockaddr_in *)(address->ai_addr))->sin_addr,
              &connection->address_string[0], sizeof(connection->address_string));
    switch (address->ai_family)
    {
        case AF_INET:
            BoltLog_info("bolt: Opening IPv4 connection to %s", &connection->address_string);
            break;
        case AF_INET6:
            BoltLog_info("bolt: Opening IPv6 connection to %s", &connection->address_string);
            break;
        default:
            BoltLog_error("bolt: Unsupported address family %d", address->ai_family);
            return -1;
    }
    connection->socket = SOCKET(address->ai_family, address->ai_socktype, 0);
    int connected = CONNECT(connection->socket, address->ai_addr, address->ai_addrlen);
    if(connected == 0)
    {
        _set_status(connection, BOLT_CONNECTED, 0);
        return 0;
    }
    _set_status(connection, BOLT_DEFUNCT, errno);
    return -1;
}

int _secure_b(struct BoltConnection* connection)
{
    BoltLog_info("bolt: Securing socket");
    SSL_library_init();
    SSL_load_error_strings();
    OpenSSL_add_all_algorithms();
    // SSL Context
    connection->ssl_context = SSL_CTX_new(TLSv1_2_client_method());
    if (connection->ssl_context == NULL || SSL_CTX_set_default_verify_paths(connection->ssl_context) != 1)
    {
        _set_status(connection, BOLT_DEFUNCT, ERR_get_error());
        return -1;
    }
    // SSL
    connection->ssl = SSL_new(connection->ssl_context);
    int linked_socket = SSL_set_fd(connection->ssl, connection->socket);
    int connected = SSL_connect(connection->ssl);
}

void _close_b(struct BoltConnection* connection)
{
    BoltLog_info("bolt: Closing connection");
    switch(connection->transport)
    {
        case BOLT_SOCKET:
        {
            SHUTDOWN(connection->socket, 2);
            break;
        }
        case BOLT_SECURE_SOCKET:
        {
            if (connection->ssl != NULL)   {
                SSL_shutdown(connection->ssl);
                SSL_free(connection->ssl);
                connection->ssl = NULL;
            }
            if (connection->ssl_context != NULL)   {
                SSL_CTX_free(connection->ssl_context);
                connection->ssl_context = NULL;
            }
            SHUTDOWN(connection->socket, 2);
            break;
        }
    }
    _set_status(connection, BOLT_DISCONNECTED, 0);
}

int _transmit_b(struct BoltConnection* connection, const char* data, int size)
{
    if (size == 0)
    {
        return 0;
    }
    int remaining = size;
    int total_sent = 0;
    while (total_sent < size)
    {
        int sent = 0;
        switch(connection->transport)
        {
            case BOLT_SOCKET:
            {
                sent = TRANSMIT(connection->socket, data, size, 0);
                break;
            }
            case BOLT_SECURE_SOCKET:
            {
                sent = TRANSMIT_S(connection->ssl, data, size, 0);
                break;
            }
        }
        if (sent >= 0)
        {
            total_sent += sent;
            remaining -= sent;
        }
        else
        {
            switch (connection->transport)
            {
                case BOLT_SOCKET:
                    _set_status(connection, BOLT_DEFUNCT, errno);
                    BoltLog_error("bolt: Socket error %d on transmit", connection->error);
                    break;
                case BOLT_SECURE_SOCKET:
                    _set_status(connection, BOLT_DEFUNCT, SSL_get_error(connection->ssl, sent));
                    BoltLog_error("bolt: SSL error %d on transmit", connection->error);
                    break;
            }
        }
    }
    BoltLog_info("bolt: Sent %d out of %d bytes", total_sent, size);
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
int _receive_b(struct BoltConnection* connection, char* buffer, int min_size, int max_size)
{
    if (min_size == 0)
    {
        return 0;
    }
    int max_remaining = max_size;
    int total_received = 0;
    while (total_received < min_size)
    {
        int received = 0;
        switch (connection->transport)
        {
            case BOLT_SOCKET:
                received = RECEIVE(connection->socket, buffer, max_remaining, 0);
                break;
            case BOLT_SECURE_SOCKET:
                received = RECEIVE_S(connection->ssl, buffer, max_remaining, 0);
                break;
        }
        if (received > 0)
        {
            total_received += received;
            max_remaining -= received;
        }
        else if (received == 0)
        {
            BoltLog_info("bolt: Detected end of transmission");
            _set_status(connection, BOLT_DISCONNECTED, 0);
            break;
        }
        else
        {
            switch (connection->transport)
            {
                case BOLT_SOCKET:
                    _set_status(connection, BOLT_DEFUNCT, errno);
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
    BoltLog_info("bolt: Received %d out of %d..%d bytes", total_received, min_size, max_size);
    return total_received;
}

int _take_b(struct BoltConnection* connection, char* buffer, int size)
{
    if (size == 0) return 0;
    int available = BoltBuffer_unloadable(connection->rx_buffer_0);
    if (size > available)
    {
        int delta = size - available;
        while (delta > 0)
        {
            int max_size = BoltBuffer_loadable(connection->rx_buffer_0);
            if (max_size == 0)
            {
                BoltBuffer_compact(connection->rx_buffer_0);
            }
            max_size = delta > max_size ? delta : max_size;
            int received = _receive_b(connection, BoltBuffer_load_target(connection->rx_buffer_0, max_size), delta, max_size);
            if (received == -1)
            {
                return -1;
            }
            // adjust the buffer extent based on the actual amount of data received
            connection->rx_buffer_0->extent = connection->rx_buffer_0->extent - max_size + received;
            delta -= received;
        }
    }
    BoltBuffer_unload(connection->rx_buffer_0, buffer, size);
    return size;
}

int _handshake_b(struct BoltConnection* connection, int32_t _1, int32_t _2, int32_t _3, int32_t _4)
{
    BoltLog_info("bolt: Performing handshake");
    BoltBuffer_load(connection->tx_buffer_1, "\x60\x60\xB0\x17", 4);
    BoltBuffer_load_int32be(connection->tx_buffer_1, _1);
    BoltBuffer_load_int32be(connection->tx_buffer_1, _2);
    BoltBuffer_load_int32be(connection->tx_buffer_1, _3);
    BoltBuffer_load_int32be(connection->tx_buffer_1, _4);
    try(BoltConnection_transmit_b(connection));
    try(_take_b(connection, BoltBuffer_load_target(connection->rx_buffer_1, 4), 4));
    BoltBuffer_unload_int32be(connection->rx_buffer_1, &connection->protocol_version);
    BoltLog_info("bolt: Using Bolt v%d", connection->protocol_version);
}

struct BoltConnection* BoltConnection_open_b(enum BoltTransport transport, const struct addrinfo* address)
{
    struct BoltConnection* connection = _create(transport);
    int opened = _open_b(connection, address);
    if (opened == -1)
    {
        return NULL;
    }
    if (transport == BOLT_SECURE_SOCKET)
    {
        int secured = _secure_b(connection);
        if (secured == -1)
        {
            return NULL;
        }
    }
    _handshake_b(connection, 1, 0, 0, 0);
    return connection;
}

void BoltConnection_close_b(struct BoltConnection* connection)
{
    _close_b(connection);
    _destroy(connection);
}

int BoltConnection_transmit_b(struct BoltConnection* connection)
{
    BoltBuffer_compact(connection->tx_buffer_1);
    switch (connection->protocol_version)
    {
        case 1:
        {
            // TODO: more chunks if size is too big
            int size = BoltBuffer_unloadable(connection->tx_buffer_1);
            while (size > 0)
            {
                char header[2];
                header[0] = (char)(size >> 8);
                header[1] = (char)(size);
                try(_transmit_b(connection, &header[0], 2));
                try(_transmit_b(connection, BoltBuffer_unload_target(connection->tx_buffer_1, size), size));
                header[0] = (char)(0);
                header[1] = (char)(0);
                try(_transmit_b(connection, &header[0], 2));
                BoltBuffer_pull_stop(connection->tx_buffer_1);
                size = BoltBuffer_unloadable(connection->tx_buffer_1);
            }
            return 0;
        }
        default:
        {
            int size = BoltBuffer_unloadable(connection->tx_buffer_1);
            while (size > 0)
            {
                try(_transmit_b(connection, BoltBuffer_unload_target(connection->tx_buffer_1, size), size));
                BoltBuffer_pull_stop(connection->tx_buffer_1);
                size = BoltBuffer_unloadable(connection->tx_buffer_1);
            }
            return 0;
        }
    }
}

int BoltConnection_receive_b(struct BoltConnection* connection)
{
    int records = 0;
    while (BoltConnection_fetch_b(connection) == 1)
    {
        records += 1;
    }
    return records;
}

int BoltConnection_fetch_b(struct BoltConnection* connection)
{
    BoltBuffer_compact(connection->rx_buffer_1);
    BoltBuffer_compact(connection->rx_buffer_0);
    switch (connection->protocol_version)
    {
        case 1:
        {
            char header[2];
            try(_take_b(connection, &header[0], 2));
            int chunk_size = char_to_int16be(header);
            while (chunk_size != 0)
            {
                try(_take_b(connection, BoltBuffer_load_target(connection->rx_buffer_1, chunk_size), chunk_size));
                try(_take_b(connection, &header[0], 2));
                chunk_size = char_to_int16be(header);
            }
            BoltProtocolV1_unload(connection, connection->current);
            return (BoltValue_type(connection->current) == BOLT_SUMMARY) ? 0 : 1;
        }
        default:
        {
            // TODO
            return -1;
        }
    }
}

struct BoltValue* BoltConnection_current(struct BoltConnection* connection)
{
    return connection->current;
}

int BoltConnection_init_b(struct BoltConnection* connection, const char* user, const char* password)
{
    BoltLog_info("bolt: Initialising connection for user '%s'", user);
    switch (connection->protocol_version)
    {
        case 1:
            BoltProtocolV1_load_init(connection, user, password);
            try(BoltConnection_transmit_b(connection));
            try(BoltConnection_receive_b(connection));
            int16_t code = BoltSummary_code(connection->current);
            switch (code)
            {
                case 0x70:  // SUCCESS
                    _set_status(connection, BOLT_READY, 0);
                    return 0;
                case 0x7F:  // FAILURE
                    _set_status(connection, BOLT_DEFUNCT, -1);
                    BoltLog_error("bolt: Initialisation failure");
                    return -1;
                default:
                    _set_status(connection, BOLT_DEFUNCT, -1);
                    BoltLog_error("bolt: Protocol violation on initialisation (summary code %d)", code);
                    return -1;
            }
        default:
            _set_status(connection, BOLT_DEFUNCT, -1);
            BoltLog_error("bolt: Protocol version unsupported");
            return -1;
    }
}

int BoltConnection_load_run(struct BoltConnection* connection, const char* statement)
{
    switch (connection->protocol_version)
    {
        case 1:
            BoltProtocolV1_load_run(connection, statement);
            return 0;
        default:
            return -1;
    }
}

int BoltConnection_load_pull(struct BoltConnection* connection, int n)
{
    switch (connection->protocol_version)
    {
        case 1:
            if (n >= 0)
            {
                return -1;
            }
            else
            {
                BoltProtocolV1_load_pull(connection);
                return 0;
            }
        default:
            return -1;
    }
}
