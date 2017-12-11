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
#include <warden.h>
#include <err.h>


#define INITIAL_TX_BUFFER_SIZE 8192
#define INITIAL_RX_BUFFER_SIZE 131072

#define char_to_int16be(array) (header[0] << 8) | (header[1]);


static const char* DEFAULT_USER_AGENT = "seabolt/1.0.0a";


struct BoltConnection* _create();
void _open(struct BoltConnection* connection, const char* address);
void _close(struct BoltConnection* connection);
void _destroy(struct BoltConnection* connection);


struct BoltConnection* _create()
{
    struct BoltConnection* connection = BoltMem_allocate(sizeof(struct BoltConnection));
    memset(connection, 0, sizeof(struct BoltConnection));
    connection->user_agent = DEFAULT_USER_AGENT;
    connection->tx_buffer = BoltBuffer_create(INITIAL_TX_BUFFER_SIZE);
    connection->rx_buffer = BoltBuffer_create(INITIAL_RX_BUFFER_SIZE);
    connection->raw_rx_buffer = BoltBuffer_create(INITIAL_RX_BUFFER_SIZE);
    connection->incoming = BoltValue_create();
}

void _open(struct BoltConnection* connection, const char* address)
{
    BoltLog_info("[CON] Opening connection to %s", address);
    assert(connection->bio != NULL);
    BIO_set_conn_hostname(connection->bio, address);
    if (BIO_do_connect(connection->bio) == 1)
    {
        connection->status = BOLT_CONNECTED;
        return;
    }

    connection->status = BOLT_CONNECTION_FAILED;
    connection->openssl_error = ERR_get_error();
    if (BIO_should_retry(connection->bio))
    {
        return _open(connection, address);
    }
}

void _close(struct BoltConnection* connection)
{
    if (connection->ssl_context != NULL)   {
        SSL_CTX_free(connection->ssl_context);
        connection->ssl_context = NULL;
    }
    BIO_free_all(connection->bio);
    connection->status = BOLT_DISCONNECTED;
}

void _destroy(struct BoltConnection* connection)
{
    connection->bio = NULL;
    BoltValue_destroy(connection->incoming);
    BoltBuffer_destroy(connection->raw_rx_buffer);
    BoltBuffer_destroy(connection->rx_buffer);
    BoltBuffer_destroy(connection->tx_buffer);
    BoltMem_deallocate(connection, sizeof(struct BoltConnection));
}

struct BoltConnection* BoltConnection_open_socket_b(const char* address)
{
    struct BoltConnection* connection =_create();

    connection->bio = BIO_new(BIO_s_connect());
    _open(connection, address);

    return connection;
}

struct BoltConnection* BoltConnection_open_secure_socket_b(const char* address)
{
    struct BoltConnection* connection =_create();

    BoltLog_info("[CON] Initialising OpenSSL");
    SSL_library_init();
    SSL_load_error_strings();
    connection->ssl_context = SSL_CTX_new(TLSv1_2_client_method());
    if (connection->ssl_context == NULL || SSL_CTX_set_default_verify_paths(connection->ssl_context) != 1)
    {
        connection->status = BOLT_CONNECTION_TLS_FAILED;
        connection->openssl_error = ERR_get_error();
        return connection;
    }

    connection->bio = BIO_new_ssl_connect(connection->ssl_context);
    _open(connection, address);

    struct ssl_st* _ssl;
    BIO_get_ssl(connection->bio, &_ssl);
    if (_ssl == NULL)
    {
        connection->status = BOLT_CONNECTION_TLS_FAILED;
        connection->openssl_error = ERR_get_error();
        return connection;
    }
    SSL_set_mode(_ssl, SSL_MODE_AUTO_RETRY);

    return connection;
}

void BoltConnection_close_b(struct BoltConnection* connection)
{
    _close(connection);
    _destroy(connection);
}

int _transmit_b(struct BoltConnection* connection, const char* data, int len)
{
    int sent = BIO_write(connection->bio, data, len);
    if (sent > 0)
    {
        BoltLog_info("[CON] Transmitted %d bytes", sent);
        return sent;
    }
    if (BIO_should_retry(connection->bio))
    {
        return _transmit_b(connection, data, len);
    }
    printf("Transmission error: %s", ERR_error_string(ERR_get_error(), NULL));
    connection->bolt_error = ERR_get_error();
    return -1;
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
                try(_transmit_b(connection, BoltBuffer_unloadTarget(connection->tx_buffer, size), size));
                header[0] = (char)(0);
                header[1] = (char)(0);
                try(_transmit_b(connection, &header[0], 2));
                BoltBuffer_pullStop(connection->tx_buffer);
                size = BoltBuffer_unloadable(connection->tx_buffer);
            }
            return 0;
        }
        default:
        {
            int size = BoltBuffer_unloadable(connection->tx_buffer);
            while (size > 0)
            {
                try(_transmit_b(connection, BoltBuffer_unloadTarget(connection->tx_buffer, size), size));
                BoltBuffer_pullStop(connection->tx_buffer);
                size = BoltBuffer_unloadable(connection->tx_buffer);
            }
            return 0;
        }
    }
}

int _receive_b(struct BoltConnection* connection, char* buffer, int size)
{
    int received = BIO_read(connection->bio, buffer, size);
    if (received > 0)
    {
        BoltLog_info("[CON] Received %d bytes", received);
        return received;
    }
    if (received < 0)
    {
        if (BIO_should_retry(connection->bio))
        {
            return _receive_b(connection, buffer, size);
        }
        printf("%s", ERR_error_string(ERR_get_error(), NULL));
        return -1;
    }
    printf("Unable to read from a closed connection.");
    return -1;
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
            int d = delta < INITIAL_RX_BUFFER_SIZE ? INITIAL_RX_BUFFER_SIZE : delta;
            int received = _receive_b(connection, BoltBuffer_loadTarget(connection->raw_rx_buffer, d), d);
            // adjust the buffer extent based on the actual amount of data received
            connection->raw_rx_buffer->extent = connection->raw_rx_buffer->extent - d + received;
            if (received == -1) { return -1; }
            delta -= received;
        }
    }
    BoltBuffer_unload(connection->raw_rx_buffer, buffer, size);
    return size;
}

int BoltConnection_receive_b(struct BoltConnection* connection)
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
                try(_take_b(connection, BoltBuffer_loadTarget(connection->rx_buffer, chunk_size), chunk_size));
                try(_take_b(connection, &header[0], 2));
                chunk_size = char_to_int16be(header);
            }
            return 0;
        }
        default:
        {
            // TODO
            return -1;
        }
    }
}

struct BoltValue* BoltConnection_fetch_b(struct BoltConnection* connection)
{
    BoltConnection_receive_b(connection);
    BoltProtocolV1_unload(connection, connection->incoming);
    return connection->incoming;
}

int BoltConnection_handshake_b(struct BoltConnection* connection, int32_t _1, int32_t _2, int32_t _3,
                               int32_t _4)
{
    BoltBuffer_load(connection->tx_buffer, "\x60\x60\xB0\x17", 4);
    BoltBuffer_load_int32be(connection->tx_buffer, _1);
    BoltBuffer_load_int32be(connection->tx_buffer, _2);
    BoltBuffer_load_int32be(connection->tx_buffer, _3);
    BoltBuffer_load_int32be(connection->tx_buffer, _4);
    try(BoltConnection_transmit_b(connection));
    try(_take_b(connection, BoltBuffer_loadTarget(connection->rx_buffer, 4), 4));
    BoltBuffer_unload_int32be(connection->rx_buffer, &connection->protocol_version);
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
            BoltProtocolV1_loadInit(connection, user, password);
            try(BoltConnection_transmit_b(connection));
            try(BoltConnection_receive_b(connection));
            BoltProtocolV1_unload(connection, connection->incoming);
//            BoltValue_dumpLine(connection->incoming);
            return 0;
        default:
            return -1;
    }
}

int BoltConnection_load_run(struct BoltConnection* connection, const char* statement)
{
    switch (connection->protocol_version)
    {
        case 1:
            BoltProtocolV1_loadRun(connection, statement);
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
            BoltProtocolV1_loadPull(connection);
            return 0;
        default:
            return -1;
    }
}
