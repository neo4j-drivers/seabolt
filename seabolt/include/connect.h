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

#include "buffer.h"
#include "values.h"


#define try(code) { int status = (code); if (status == -1) return status; }


enum BoltConnectionStatus
{
    BOLT_DISCONNECTED,
    BOLT_CONNECTED,
    BOLT_CONNECTION_TLS_FAILED,
    BOLT_CONNECTION_FAILED,

};
struct BoltConnection
{
    enum BoltConnectionStatus status;
    struct bio_st* bio;
    struct ssl_ctx_st* ssl_context;
    int32_t protocol_version;
    const char* user_agent;
    struct BoltBuffer* buffer;
    unsigned long bolt_error;
    unsigned long openssl_error;
    struct BoltValue* incoming; // holder for incoming messages (one at a time so we can reuse this)
};


struct BoltConnection* BoltConnection_openSocket(const char* host, int port);

struct BoltConnection* BoltConnection_openSecureSocket(const char* host, int port);

void BoltConnection_close(struct BoltConnection* connection);

int BoltConnection_transmit(struct BoltConnection* connection);

int BoltConnection_receive(struct BoltConnection* connection);

struct BoltValue* BoltConnection_fetch(struct BoltConnection* connection);

int32_t BoltConnection_handshake(struct BoltConnection* connection, int32_t first, int32_t second, int32_t third, int32_t fourth);

int BoltConnection_init(struct BoltConnection* connection, const char* user, const char* password);

int BoltConnection_loadRun(struct BoltConnection* connection, const char*);

int BoltConnection_loadPull(struct BoltConnection* connection);


#endif // SEABOLT_CONNECT
