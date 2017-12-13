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
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <warden.h>
#include <err.h>
#include <netdb.h>


#define INITIAL_TX_BUFFER_SIZE 8192
#define INITIAL_RX_BUFFER_SIZE 8192

#define char_to_int16be(array) (header[0] << 8) | (header[1]);


static const char* DEFAULT_USER_AGENT = "seabolt/1.0.0a";


struct BoltConnection* _create(enum BoltTransport transport);
void _destroy(struct BoltConnection* connection);
int _open_b(struct BoltConnection* connection, const struct addrinfo* address);
void _close_b(struct BoltConnection* connection);


struct BoltConnection* _create(enum BoltTransport transport)
{
    struct BoltConnection* connection = BoltMem_allocate(sizeof(struct BoltConnection));
    memset(connection, 0, sizeof(struct BoltConnection));
    connection->transport = transport;
    connection->user_agent = DEFAULT_USER_AGENT;
    connection->tx_buffer = BoltBuffer_create(INITIAL_TX_BUFFER_SIZE);
    connection->rx_buffer = BoltBuffer_create(INITIAL_RX_BUFFER_SIZE);
    connection->raw_rx_buffer = BoltBuffer_create(INITIAL_RX_BUFFER_SIZE);
    connection->current = BoltValue_create();
}

void _destroy(struct BoltConnection* connection)
{
    BoltValue_destroy(connection->current);
    BoltBuffer_destroy(connection->raw_rx_buffer);
    BoltBuffer_destroy(connection->rx_buffer);
    BoltBuffer_destroy(connection->tx_buffer);
    BoltMem_deallocate(connection, sizeof(struct BoltConnection));
}

int _open_b(struct BoltConnection* connection, const struct addrinfo* address)
{
    connection->socket = socket(address->ai_family, address->ai_socktype, 0);
    BoltLog_info("[NET] Opening connection to %s", address->ai_canonname);
    int connected = connect(connection->socket, address->ai_addr, address->ai_addrlen);
    if(connected == 0)
    {
        connection->status = BOLT_CONNECTED;
        return 0;
    }
    switch(errno)
    {
        default:
            connection->status = BOLT_CONNECTION_FAILED;
    }
    return -1;
}

void _close_b(struct BoltConnection* connection)
{
    switch(connection->transport)
    {
        case BOLT_SOCKET:
        {
            shutdown(connection->socket, 2);
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
            shutdown(connection->socket, 2);
            break;
        }
    }
    connection->status = BOLT_DISCONNECTED;
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
                sent = (int)(send(connection->socket, data, (size_t)(size), 0));
                break;
            }
            case BOLT_SECURE_SOCKET:
            {
                sent = SSL_write(connection->ssl, data, size);
                break;
            }
        }
        if (sent > 0)
        {
            total_sent += sent;
            remaining -= sent;
        }
        else if (sent == 0)
        {
            // TODO: can this happen?
            connection->status = BOLT_SENT_ZERO;
            break;
        }
        else
        {
            switch (connection->transport)
            {
                case BOLT_SOCKET:
                    switch (errno)
                    {
                        default:
                            connection->status = BOLT_SEND_FAILED;
                            return -1;
                    }
                case BOLT_SECURE_SOCKET:
                    switch (SSL_get_error(connection->ssl, sent))
                    {
                        default:
                            connection->status = BOLT_SEND_FAILED;
                            return -1;
                    }
            }
        }
    }
    BoltLog_info("[NET] Sent %d bytes (requested %d)", total_sent, size);
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
                received = (int)(recv(connection->socket, buffer, (size_t)(max_remaining), 0));
                break;
            case BOLT_SECURE_SOCKET:
                received = SSL_read(connection->ssl, buffer, max_remaining);
                break;
        }
        if (received > 0)
        {
            total_received += received;
            max_remaining -= received;
        }
        else if (received == 0)
        {
            connection->status = BOLT_RECEIVED_ZERO;
            break;
        }
        else
        {
            switch (connection->transport)
            {
                case BOLT_SOCKET:
                    switch (errno)
                    {
                        case ECONNREFUSED:
                            connection->status = BOLT_CONNECTION_REFUSED;
                            return -1;
                        default:
                            connection->status = BOLT_RECEIVE_FAILED;
                            return -1;
                    }
                case BOLT_SECURE_SOCKET:
                    switch (SSL_get_error(connection->ssl, received))
                    {
                        default:
                            connection->status = BOLT_RECEIVE_FAILED;
                            return -1;
                    }
            }
        }
    }
    BoltLog_info("[NET] Received %d bytes (requested %d..%d)", total_received, min_size, max_size);
    return total_received;
}

int _take_b(struct BoltConnection* connection, char* buffer, int size)
{
    if (size == 0) return 0;
    int available = BoltBuffer_unloadable(connection->raw_rx_buffer);
    if (size > available)
    {
        int delta = size - available;
        while (delta > 0)
        {
            int max_size = BoltBuffer_loadable(connection->raw_rx_buffer);
            if (max_size == 0)
            {
                BoltBuffer_compact(connection->raw_rx_buffer);
            }
            max_size = delta > max_size ? delta : max_size;
            int received = _receive_b(connection, BoltBuffer_load_target(connection->raw_rx_buffer, max_size), delta, max_size);
            if (received == 0)
            {
                connection->status = BOLT_RECEIVED_ZERO;
                return 0;
            }
            // adjust the buffer extent based on the actual amount of data received
            connection->raw_rx_buffer->extent = connection->raw_rx_buffer->extent - max_size + received;
            if (received == -1) { return -1; }
            delta -= received;
        }
    }
    BoltBuffer_unload(connection->raw_rx_buffer, buffer, size);
    return size;
}

int _handshake_b(struct BoltConnection* connection, int32_t _1, int32_t _2, int32_t _3, int32_t _4)
{
    BoltBuffer_load(connection->tx_buffer, "\x60\x60\xB0\x17", 4);
    BoltBuffer_load_int32be(connection->tx_buffer, _1);
    BoltBuffer_load_int32be(connection->tx_buffer, _2);
    BoltBuffer_load_int32be(connection->tx_buffer, _3);
    BoltBuffer_load_int32be(connection->tx_buffer, _4);
    try(BoltConnection_transmit_b(connection));
    try(_take_b(connection, BoltBuffer_load_target(connection->rx_buffer, 4), 4));
    BoltBuffer_unload_int32be(connection->rx_buffer, &connection->protocol_version);
    BoltLog_info("[NET] Using Bolt v%d", connection->protocol_version);
}

struct BoltConnection* BoltConnection_open_b(enum BoltTransport transport, const struct addrinfo* address)
{
    struct BoltConnection* connection = _create(transport);
    switch(transport)
    {
        case BOLT_SOCKET:
        {
            _open_b(connection, address);
            _handshake_b(connection, 1, 0, 0, 0);
            return connection;
        }
        case BOLT_SECURE_SOCKET:
        {
            _open_b(connection, address);
            if (connection->status != BOLT_CONNECTED)
            {
                return connection;
            }
            BoltLog_info("[NET] Securing socket");
            SSL_library_init();
            SSL_load_error_strings();
            OpenSSL_add_all_algorithms();
            // SSL Context
            connection->ssl_context = SSL_CTX_new(TLSv1_2_client_method());
            if (connection->ssl_context == NULL || SSL_CTX_set_default_verify_paths(connection->ssl_context) != 1)
            {
                connection->status = BOLT_CONNECTION_TLS_FAILED;
                connection->openssl_error = ERR_get_error();
                return connection;
            }
            // SSL
            connection->ssl = SSL_new(connection->ssl_context);
            int linked_socket = SSL_set_fd(connection->ssl, connection->socket);
            int connected = SSL_connect(connection->ssl);
            _handshake_b(connection, 1, 0, 0, 0);
            return connection;
        }
    }
}

void BoltConnection_close_b(struct BoltConnection* connection)
{
    _close_b(connection);
    _destroy(connection);
}

int BoltConnection_transmit_b(struct BoltConnection* connection)
{
    BoltBuffer_compact(connection->tx_buffer);
    switch (connection->protocol_version)
    {
        case 1:
        {
            // TODO: more chunks if size is too big
            int size = BoltBuffer_unloadable(connection->tx_buffer);
            while (size > 0)
            {
                char header[2];
                header[0] = (char)(size >> 8);
                header[1] = (char)(size);
                try(_transmit_b(connection, &header[0], 2));
                try(_transmit_b(connection, BoltBuffer_unload_target(connection->tx_buffer, size), size));
                header[0] = (char)(0);
                header[1] = (char)(0);
                try(_transmit_b(connection, &header[0], 2));
                BoltBuffer_pull_stop(connection->tx_buffer);
                size = BoltBuffer_unloadable(connection->tx_buffer);
            }
            return 0;
        }
        default:
        {
            int size = BoltBuffer_unloadable(connection->tx_buffer);
            while (size > 0)
            {
                try(_transmit_b(connection, BoltBuffer_unload_target(connection->tx_buffer, size), size));
                BoltBuffer_pull_stop(connection->tx_buffer);
                size = BoltBuffer_unloadable(connection->tx_buffer);
            }
            return 0;
        }
    }
}

int BoltConnection_receive_b(struct BoltConnection* connection)
{
    int records = 0;
    while (BoltConnection_fetch_b(connection))
    {
        records += 1;
    }
    return records;
}

int BoltConnection_fetch_b(struct BoltConnection* connection)
{
    BoltBuffer_compact(connection->rx_buffer);
    BoltBuffer_compact(connection->raw_rx_buffer);
    switch (connection->protocol_version)
    {
        case 1:
        {
            char header[2];
            try(_take_b(connection, &header[0], 2));
            int chunk_size = char_to_int16be(header);
            while (chunk_size != 0)
            {
                try(_take_b(connection, BoltBuffer_load_target(connection->rx_buffer, chunk_size), chunk_size));
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

/**
 * Initialise with basic auth.
 *
 * @return
 */
int BoltConnection_init_b(struct BoltConnection* connection, const char* user, const char* password)
{
    switch (connection->protocol_version)
    {
        case 1:
            BoltProtocolV1_load_init(connection, user, password);
            try(BoltConnection_transmit_b(connection));
            try(BoltConnection_receive_b(connection));
            BoltProtocolV1_unload(connection, connection->current);
//            _dump(connection->current);
            switch(BoltSummary_code(connection->current))
            {
                case 0x70:  // SUCCESS
                    BoltLog_info("[NET] INIT succeeded for user %s", user);
                    connection->status = BOLT_READY;
                    return 0;
                case 0x7F:  // FAILURE
                    BoltLog_error("[NET] INIT failed for user %s", user);
                    connection->status = BOLT_INIT_FAILED;
                    return -1;
                default:
                    BoltLog_error("[NET] Protocol violation");
                    connection->status = BOLT_PROTOCOL_VIOLATION;
                    return -1;
            }
        default:
            BoltLog_error("[NET] Protocol violation");
            connection->status = BOLT_PROTOCOL_VERSION_UNSUPPORTED;
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

int BoltConnection_load_pull(struct BoltConnection* connection)
{
    switch (connection->protocol_version)
    {
        case 1:
            BoltProtocolV1_load_pull(connection);
            return 0;
        default:
            return -1;
    }
}
