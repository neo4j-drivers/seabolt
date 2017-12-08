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
#include <logging.h>
#include <err.h>


#define return_if(predicate, bolt_err, lib_err, return_value)       \
    if (predicate)                                                  \
    {                                                               \
        connection->bolt_error = (bolt_err);                        \
        connection->library_error = (lib_err);                      \
        return (return_value);                                      \
    }


#define char_to_int16be(array) (header[0] << 8) | (header[1]);


static const char* DEFAULT_USER_AGENT = "seabolt/1.0.0a";


int _open(struct BoltConnection* connection, const char* host, int port)
{
    connection->host = host;
    connection->port = port;
    char address[strlen(connection->host) + 6];
    sprintf(&address[0], "%s:%d", connection->host, connection->port);
    BIO_set_conn_hostname(connection->bio, address);
    return_if(BIO_do_connect(connection->bio) <= 0, BOLT_SOCKET_ERROR, ERR_get_error(), EXIT_FAILURE);
    if (connection->_ssl != NULL)
    {
        if (SSL_get_verify_result(connection->_ssl) != X509_V_OK)
        {
            SSL_set_verify_result(connection->_ssl, X509_V_OK);
        }
    }
    connection->buffer = BoltBuffer_create(8192);
    connection->protocol_version = 0;
    connection->user_agent = DEFAULT_USER_AGENT;
    connection->incoming = BoltValue_create();

}

struct BoltConnection* BoltConnection_openSocket(const char* host, int port)
{
    struct BoltConnection* connection = BoltMem_allocate(sizeof(struct BoltConnection));
    connection->bio = BIO_new(BIO_s_connect());
    return_if(connection->bio == NULL, BOLT_SOCKET_ERROR, 0, NULL);
    _open(connection, host, port);
    return connection;
}

struct BoltConnection* BoltConnection_openSecureSocket(const char* host, int port)
{
    struct BoltConnection* connection = BoltMem_allocate(sizeof(struct BoltConnection));
    // Ensure SSL has been initialised
    SSL_library_init();
    SSL_load_error_strings();

    // create a new TLS 1.2 context
    connection->ssl_context = SSL_CTX_new(TLSv1_2_client_method());
    return_if(connection->ssl_context == NULL, BOLT_TLS_ERROR, ERR_get_error(), NULL);

    // set CA cert store to the defaults
    return_if(SSL_CTX_set_default_verify_paths(connection->ssl_context) <= 0,
              BOLT_TLS_ERROR, ERR_get_error(), NULL);

    connection->bio = BIO_new_ssl_connect(connection->ssl_context);
    return_if(connection->bio == NULL, BOLT_TLS_ERROR, ERR_get_error(), NULL);

    BIO_get_ssl(connection->bio, &connection->_ssl);
    return_if(connection->_ssl == NULL, BOLT_TLS_ERROR, ERR_get_error(), NULL);

    SSL_set_mode(connection->_ssl, SSL_MODE_AUTO_RETRY);

    _open(connection, host, port);
    return connection;
}

void BoltConnection_close(struct BoltConnection* connection)
{
    BoltBuffer_destroy(connection->buffer);
    BoltValue_destroy(connection->incoming);
    BIO_free_all(connection->bio);
    SSL_CTX_free(connection->ssl_context);
    BoltMem_deallocate(connection, sizeof(struct BoltConnection));
}

int _transmit(struct BoltConnection* connection, const char* data, int len)
{
    int sent = BIO_write(connection->bio, data, len);
    if (sent > 0)
    {
        log("[CON] Transmitted %d bytes", sent);
        return sent;
    }
    if (BIO_should_retry(connection->bio))
    {
        return _transmit(connection, data, len);
    }
    printf("Transmission error: %s", ERR_error_string(ERR_get_error(), NULL));
    connection->bolt_error = ERR_get_error();
    return -1;
}

int BoltConnection_transmit(struct BoltConnection* connection)
{
    switch (connection->protocol_version)
    {
        case 1:
        {
            // TODO: more chunks if size is too big
            int size = BoltBuffer_unloadable(connection->buffer);
            while (size > 0)
            {
                char header[2];
                header[0] = (char)(size >> 8);
                header[1] = (char)(size);
                try(_transmit(connection, &header[0], 2));
                try(_transmit(connection, BoltBuffer_unloadTarget(connection->buffer, size), size));
                header[0] = (char)(0);
                header[1] = (char)(0);
                try(_transmit(connection, &header[0], 2));
                BoltBuffer_pullStop(connection->buffer);
                size = BoltBuffer_unloadable(connection->buffer);
            }
            return 0;
        }
        default:
        {
            int size = BoltBuffer_unloadable(connection->buffer);
            while (size > 0)
            {
                try(_transmit(connection, BoltBuffer_unloadTarget(connection->buffer, size), size));
                BoltBuffer_pullStop(connection->buffer);
                size = BoltBuffer_unloadable(connection->buffer);
            }
            return 0;
        }
    }
}

int _receive(struct BoltConnection* connection, char* buffer, int size)
{
    int received = BIO_read(connection->bio, buffer, size);
    if (received == 0)
    {
        printf("Unable to read from a closed connection.");
        return -1;
    }
    if (received < 0)
    {
        if (BIO_should_retry(connection->bio))
        {
            return _receive(connection, buffer, size);
        }
        printf("%s", ERR_error_string(ERR_get_error(), NULL));
        return -1;
    }
    log("[CON] Received %d bytes", received);
    return received;
}

int BoltConnection_receive(struct BoltConnection* connection)
{
    switch (connection->protocol_version)
    {
        case 1:
        {
            char header[2];
            try(_receive(connection, &header[0], 2));
            int chunk_size = char_to_int16be(header);
            while (chunk_size != 0)
            {
                try(_receive(connection, BoltBuffer_loadTarget(connection->buffer, chunk_size), chunk_size));
                try(_receive(connection, &header[0], 2));
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

struct BoltValue* BoltConnection_fetch(struct BoltConnection* connection)
{
    BoltConnection_receive(connection);
    BoltProtocolV1_unload(connection, connection->incoming);
    return connection->incoming;
}

int BoltConnection_handshake(struct BoltConnection* connection, int32_t first, int32_t second, int32_t third, int32_t fourth)
{
    BoltBuffer_load(connection->buffer, "\x60\x60\xB0\x17", 4);
    BoltBuffer_load_int32be(connection->buffer, first);
    BoltBuffer_load_int32be(connection->buffer, second);
    BoltBuffer_load_int32be(connection->buffer, third);
    BoltBuffer_load_int32be(connection->buffer, fourth);
    try(BoltConnection_transmit(connection));
    try(_receive(connection, BoltBuffer_loadTarget(connection->buffer, 4), 4));
    BoltBuffer_unload_int32be(connection->buffer, &connection->protocol_version);
}

/**
 * Initialise with basic auth.
 *
 * @return
 */
int BoltConnection_init(struct BoltConnection* connection, const char* user, const char* password)
{
    switch (connection->protocol_version)
    {
        case 1:
            BoltProtocolV1_loadInit(connection, user, password);
            try(BoltConnection_transmit(connection));
            try(BoltConnection_receive(connection));
            BoltProtocolV1_unload(connection, connection->incoming);
//            BoltValue_dumpLine(connection->incoming);
            return 0;
        default:
            return -1;
    }
}

int BoltConnection_loadRun(struct BoltConnection* connection, const char* statement)
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

int BoltConnection_loadPull(struct BoltConnection* connection)
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
