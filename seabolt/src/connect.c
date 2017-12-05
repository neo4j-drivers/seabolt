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

#include "connect.h"
#include "mem.h"


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
    connection->host = host;
    connection->port = port;
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
    connection->host = host;
    connection->port = port;
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

int BoltConnection_receive(BoltConnection* connection, void* buffer, int len, int flags)
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
            return BoltConnection_receive(connection, buffer, len, flags);
        }
        printf("%s", ERR_error_string(ERR_get_error(), NULL));
        return -1;
    }

    printf("%d received\n", res);
    return res;
}

void int32be_write(char* buffer, int32_t value)
{
    buffer[0] = (char)(value >> 24);
    buffer[1] = (char)(value >> 16);
    buffer[2] = (char)(value >> 8);
    buffer[3] = (char)(value);
}

int32_t int32be_read(const char* buffer)
{
    return (buffer[0] << 24) |
           (buffer[1] << 16) |
           (buffer[2] << 8) |
           (buffer[3]);
}

int BoltConnection_handshake(BoltConnection* connection, int32_t first, int32_t second, int32_t third, int32_t fourth)
{
    char buffer[20];
    memcpy(&buffer, "\x60\x60\xB0\x17", 4);
    int32be_write(&buffer[4], first);
    int32be_write(&buffer[8], second);
    int32be_write(&buffer[12], third);
    int32be_write(&buffer[16], fourth);
    BoltConnection_transmit(connection, &buffer, 20);
    BoltConnection_receive(connection, &buffer, 4, 0);
    connection->protocol_version = int32be_read(&buffer[0]);
    // TODO: return codes
}
