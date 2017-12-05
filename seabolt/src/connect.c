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
#include <assert.h>

#include "buffer.h"
#include "connect.h"
#include "mem.h"


static const char* DEFAULT_USER_AGENT = "seabolt/1.0.0a";


int _connect(BoltConnection* connection)
{
    char host_port[strlen(connection->host) + 6];
    sprintf(&host_port[0], "%s:%d", connection->host, connection->port);

    BIO_set_conn_hostname(connection->bio, host_port);

    if (BIO_do_connect(connection->bio) <= 0)
    {
        return EXIT_FAILURE;
//        throw runtime_error("unable to establish a connection to " + hostPort + ". Error is: " + basic_string<char>(
//                ERR_error_string(ERR_get_error(), nullptr)));
    }

    if (connection->_ssl != NULL)
    {
        if (SSL_get_verify_result(connection->_ssl) != X509_V_OK)
        {
            SSL_set_verify_result(connection->_ssl, X509_V_OK);
        }
    }
}

BoltConnection* BoltConnection_openSocket(const char* host, int port)
{
    BoltConnection* connection = BoltMem_allocate(sizeof(BoltConnection));
    connection->user_agent = DEFAULT_USER_AGENT;
    connection->host = host;
    connection->port = port;
    connection->buffer = BoltBuffer_create(8192);
    connection->bio = BIO_new(BIO_s_connect());
    if (connection->bio == NULL)
    {
//            throw runtime_error("unable to initialize connection structures. Error is: " +
//                                basic_string<char>(ERR_error_string(ERR_get_error(), nullptr)));
        return NULL;
    }
    _connect(connection);
    return connection;
}

BoltConnection* BoltConnection_openSecureSocket(const char* host, int port)
{
    BoltConnection* connection = BoltMem_allocate(sizeof(BoltConnection));
    connection->user_agent = DEFAULT_USER_AGENT;
    connection->host = host;
    connection->port = port;
    connection->buffer = BoltBuffer_create(8192);
    // Ensure SSL has been initialised
    SSL_library_init();
    SSL_load_error_strings();

    // create a new TLS 1.2 context
    connection->ssl_context = SSL_CTX_new(TLSv1_2_client_method());
    if (connection->ssl_context == NULL)
    {
        printf("Error: %s\n", ERR_reason_error_string(ERR_get_error()));
        return NULL;
//        throw runtime_error("SSL context initialisation failed. Error is: " + basic_string<char>(
//                ERR_error_string(ERR_get_error(), nullptr)));
    }

    // set CA cert store to the defaults
    if (SSL_CTX_set_default_verify_paths(connection->ssl_context) <= 0)
    {
        return NULL;
//        throw runtime_error("Default trusted CA store could not be loaded. Error is: " + basic_string<char>(
//                ERR_error_string(ERR_get_error(), nullptr)));
    }

    connection->bio = BIO_new_ssl_connect(connection->ssl_context);
    if (connection->bio == NULL)
    {
        return NULL;
//        throw runtime_error("unable to initialize connection structures. Error is: " + basic_string<char>(
//                ERR_error_string(ERR_get_error(), nullptr)));
    }

    BIO_get_ssl(connection->bio, &connection->_ssl);
    if (connection->_ssl == NULL)
    {
        return NULL;
//        throw runtime_error("unable to initialize SSL structures. Error is: " + basic_string<char>(
//                ERR_error_string(ERR_get_error(), nullptr)));
    }

    SSL_set_mode(connection->_ssl, SSL_MODE_AUTO_RETRY);

    _connect(connection);
    return connection;
}

void BoltConnection_close(BoltConnection* connection)
{
    BoltBuffer_destroy(connection->buffer);
    BIO_free_all(connection->bio);
    SSL_CTX_free(connection->ssl_context);
    BoltMem_deallocate(connection, sizeof(BoltConnection));
}

int BoltConnection_transmit(BoltConnection* connection, const void* buffer, int size)
{
    int sent = BIO_write(connection->bio, buffer, size);
    if (sent > 0)
    {
        printf("Transmitted %d bytes\n", sent);
        return sent;
    }
    if (BIO_should_retry(connection->bio))
    {
        return BoltConnection_transmit(connection, buffer, size);
    }
    printf("Transmission error: %s", ERR_error_string(ERR_get_error(), NULL));
    return -1;
}

int BoltConnection_receive(BoltConnection* connection, void* buffer, int len)
{
    int res = BIO_read(connection->bio, buffer, len);

    if (res == 0)
    {
        printf("Unable to read from a closed connection.");
        return -1;
    }
    if (res < 0)
    {
        if (BIO_should_retry(connection->bio))
        {
            return BoltConnection_receive(connection, buffer, len);
        }
        printf("%s", ERR_error_string(ERR_get_error(), NULL));
        return -1;
    }

    printf("%d received\n", res);
    return res;
}

int BoltConnection_handshake(BoltConnection* connection, int32_t first, int32_t second, int32_t third, int32_t fourth)
{
    BoltBuffer_load_bytes(connection->buffer, "\x60\x60\xB0\x17", 4);
    BoltBuffer_load_int32be(connection->buffer, first);
    BoltBuffer_load_int32be(connection->buffer, second);
    BoltBuffer_load_int32be(connection->buffer, third);
    BoltBuffer_load_int32be(connection->buffer, fourth);
    BoltConnection_transmit(connection, BoltBuffer_unload(connection->buffer, 20), 20);
    BoltConnection_receive(connection, BoltBuffer_load(connection->buffer, 4), 4);
    connection->protocol_version = BoltBuffer_unload_int32be(connection->buffer);
    // TODO: return codes
}

int _init1(BoltConnection* connection, const char* user, const char* password)
{
    assert(strlen(connection->user_agent) < 0x100);
    assert(strlen(user) < 0x100);
    assert(strlen(password) < 0x100);
    char buffer[8192];
    memcpy(&buffer[0], "\x00\x00\xB2\x01", 4);
    // TODO: pretty much all of this...
}

/**
 * Initialise with basic auth.
 *
 * @return
 */
int BoltConnection_init(BoltConnection* connection, const char* user, const char* password)
{
    switch(connection->protocol_version)
    {
        case 1:
            return _init1(connection, user, password);
        default:
            return -1;
    }
}