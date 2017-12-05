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

#ifndef SEABOLT_CONNECT
#define SEABOLT_CONNECT

#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/ssl.h>


typedef struct
{
    const char* host;
    int port;
    struct ssl_ctx_st* ssl_context;
    struct ssl_st* _ssl;
    struct bio_st* bio;
    int32_t protocol_version;
} BoltConnection;


BoltConnection* BoltConnection_openSocket(const char* host, int port);

BoltConnection* BoltConnection_openSecureSocket(const char* host, int port);

void BoltConnection_close(BoltConnection* connection);

int BoltConnection_transmit(BoltConnection* connection, const void* buffer, int size);

int BoltConnection_receive(BoltConnection* connection, void* buffer, int len, int flags);

int32_t BoltConnection_handshake(BoltConnection* connection, int32_t first, int32_t second, int32_t third, int32_t fourth);


#endif // SEABOLT_CONNECT
