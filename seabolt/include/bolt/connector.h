/*
 * Copyright (c) 2002-2018 "Neo4j,"
 * Neo4j Sweden AB [http://neo4j.com]
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
#ifndef SEABOLT_ALL_CONNECTOR_H
#define SEABOLT_ALL_CONNECTOR_H

#include "bolt/logging.h"
#include "bolt/connections.h"
#include "bolt/address-resolver.h"

enum BoltConnectorMode {
    BOLT_DIRECT = 0,
    BOLT_ROUTING = 1
};

enum BoltAccessMode {
    BOLT_ACCESS_MODE_READ = 1,
    BOLT_ACCESS_MODE_WRITE = 2
};

struct BoltConfig {
    enum BoltConnectorMode mode;
    enum BoltTransport transport;
    struct BoltTrust* trust;
    char* user_agent;
    struct BoltValue* routing_context;
    struct BoltAddressResolver* address_resolver;
    struct BoltLog* log;
    int max_pool_size;
    int max_connection_lifetime;
    int max_connection_acquire_time;
    struct BoltSocketOptions* sock_opts;
};

struct BoltConnector {
    const struct BoltAddress* address;
    const struct BoltValue* auth_token;
    const struct BoltConfig* config;
    void* pool_state;
};

struct BoltConnectionResult {
    struct BoltConnection* connection;
    enum BoltConnectionStatus connection_status;
    int connection_error;
    char* connection_error_ctx;
};

#ifdef __cplusplus
#define CONNECTION_RESULT_SUCCESS(connection) { connection, BOLT_READY,  BOLT_SUCCESS, NULL }
#define CONNECTION_RESULT_ERROR(code, context) { NULL, BOLT_DISCONNECTED, code, context }
#else
#define CONNECTION_RESULT_SUCCESS(connection) (struct BoltConnectionResult) { connection, BOLT_READY,  BOLT_SUCCESS, NULL }
#define CONNECTION_RESULT_ERROR(code, context) (struct BoltConnectionResult) { NULL, BOLT_DISCONNECTED, code, context }
#endif

PUBLIC struct BoltConnector*
BoltConnector_create(struct BoltAddress* address, struct BoltValue* auth_token, struct BoltConfig* config);

PUBLIC void BoltConnector_destroy(struct BoltConnector* connector);

PUBLIC struct BoltConnectionResult BoltConnector_acquire(struct BoltConnector* connector, enum BoltAccessMode mode);

PUBLIC void BoltConnector_release(struct BoltConnector* connector, struct BoltConnection* connection);

#endif //SEABOLT_ALL_CONNECTOR_H
