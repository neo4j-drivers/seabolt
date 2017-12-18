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

/**
 * @file
 */

#ifndef SEABOLT_CONNECT
#define SEABOLT_CONNECT

#include <netdb.h>


#define try(code) { int status = (code); if (status == -1) { return status; } }


enum BoltTransport
{
    BOLT_SECURE_SOCKET,
    BOLT_INSECURE_SOCKET,
};

enum BoltConnectionStatus
{
    BOLT_DISCONNECTED,      // not connected
    BOLT_CONNECTED,         // connected but not authenticated
    BOLT_READY,             // connected and authenticated
    BOLT_FAILED,            // recoverable failure
    BOLT_DEFUNCT,           // unrecoverable failure
};

struct BoltConnection
{
    enum BoltTransport transport;
    enum BoltConnectionStatus status;
    char address_string[INET6_ADDRSTRLEN];
    int error;
    int error_s;
    // TODO: better storage for various assorted error codes
    int socket;
    struct ssl_ctx_st* ssl_context;
    struct ssl_st* ssl;
    int32_t protocol_version;
    const char* user_agent;
    // TODO raw_tx_buffer with chunks ("stop" moves data across buffers)
    struct BoltBuffer* tx_buffer_1;     // transmit buffer without chunks
    struct BoltBuffer* tx_buffer_0;     // transmit buffer with chunks
    struct BoltBuffer* rx_buffer_0;     // receive buffer with chunks
    struct BoltBuffer* rx_buffer_1;     // receive buffer without chunks
    unsigned long bolt_error;
    unsigned long openssl_error;

    int requests_queued;
    int requests_running;

    struct BoltValue* run;
    struct BoltValue* cypher_statement;
    struct BoltValue* cypher_parameters;
    struct BoltValue* discard;
    struct BoltValue* pull;
    struct BoltValue* received; // holder for received messages (one at a time so we can reuse this)
};


/**
 * Open a connection to a Bolt server.
 *
 * @param transport
 * @param address
 * @return
 */
struct BoltConnection* BoltConnection_open_b(enum BoltTransport transport, const struct addrinfo* address);

/**
 * Close a connection.
 *
 * @param connection
 */
void BoltConnection_close_b(struct BoltConnection* connection);

/**
 * Transmit all queued outgoing messages.
 *
 * @param connection
 * @return
 */
int BoltConnection_transmit_b(struct BoltConnection* connection);

/**
 * Receive all outstanding responses.
 *
 * @param connection
 * @return
 */
int BoltConnection_receive_b(struct BoltConnection* connection);

/**
 * Receive the next response in its entirety, up to and including the
 * trailing summary.
 *
 * @param connection
 * @return
 */
int BoltConnection_receive_summary_b(struct BoltConnection* connection);

/**
 * Receive the next message and load into the "current" slot.
 *
 * @param connection
 * @return
 */
int BoltConnection_receive_one_b(struct BoltConnection* connection);

/**
 * Initialise the connection and authenticate using the basic
 * authentication scheme.
 *
 * @return
 */
int BoltConnection_init_b(struct BoltConnection* connection, const char* user, const char* password);

int BoltConnection_set_statement(struct BoltConnection* connection, const char* statement, size_t size);

int BoltConnection_resize_parameters(struct BoltConnection* connection, int32_t size);

int BoltConnection_load_run(struct BoltConnection* connection);

int BoltConnection_load_discard(struct BoltConnection* connection, int32_t n);

int BoltConnection_load_pull(struct BoltConnection* connection, int32_t n);

#endif // SEABOLT_CONNECT
