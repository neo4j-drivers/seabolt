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

#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <netdb.h>

#include "buffer.h"
#include "values.h"


#define try(code) { int status = (code); if (status == -1) return status; }


enum BoltTransport
{
    BOLT_SOCKET,
    BOLT_SECURE_SOCKET,
};

enum BoltConnectionStatus
{
    BOLT_DISCONNECTED,
    BOLT_CONNECTED,
    BOLT_CONNECTION_TLS_FAILED,
    BOLT_CONNECTION_FAILED,
    BOLT_RECEIVE_FAILED,
    BOLT_CONNECTION_REFUSED,
    BOLT_RECEIVED_ZERO,
    BOLT_INIT_FAILED,
    BOLT_PROTOCOL_VIOLATION,
    BOLT_READY,
    BOLT_PROTOCOL_VERSION_UNSUPPORTED,
    BOLT_SENT_ZERO,
    BOLT_SEND_FAILED,
};

struct BoltConnection
{
    enum BoltTransport transport;
    enum BoltConnectionStatus status;
    int socket;
    struct ssl_ctx_st* ssl_context;
    struct ssl_st* ssl;
    int32_t protocol_version;
    const char* user_agent;
    struct BoltBuffer* tx_buffer;       // transmit buffer
    struct BoltBuffer* rx_buffer;       // receive buffer after chunk processing
    struct BoltBuffer* raw_rx_buffer;   // receive buffer before chunk processing
    unsigned long bolt_error;
    unsigned long openssl_error;
    struct BoltValue* current; // holder for current messages (one at a time so we can reuse this)
};


struct BoltConnection* BoltConnection_open_b(enum BoltTransport transport, const struct addrinfo* address);

void BoltConnection_close_b(struct BoltConnection* connection);

/**
 * Transmit all queued outgoing data.
 *
 * @param connection
 * @return
 */
int BoltConnection_transmit_b(struct BoltConnection* connection);

/**
 * Receive the next response in its entirety, up to and including the
 * trailing summary.
 *
 * @param connection
 * @return
 */
int BoltConnection_receive_b(struct BoltConnection* connection);

/**
 * Receive the next message and load into the "current" slot.
 *
 * @param connection
 * @return
 */
int BoltConnection_fetch_b(struct BoltConnection* connection);

struct BoltValue* BoltConnection_current(struct BoltConnection* connection);

int BoltConnection_init_b(struct BoltConnection* connection, const char* user, const char* password);

int BoltConnection_load_run(struct BoltConnection* connection, const char*);

int BoltConnection_load_pull(struct BoltConnection* connection);


#endif // SEABOLT_CONNECT
