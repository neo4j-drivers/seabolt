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

#include "bolt/connections.h"

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
    const char* user_agent;
    const struct BoltValue* auth_token;
    const struct BoltValue* routing_context;
    uint32_t max_pool_size;
};

struct BoltConnector {
    const struct BoltAddress* address;
    const struct BoltConfig* config;
    void* pool_state;
};

struct BoltConnectionResult {
    struct BoltConnection* connection;
    enum BoltConnectionStatus connection_status;
    enum BoltConnectionError connection_error;
    char* connection_error_ctx;
};

struct BoltConnector* BoltConnector_create(struct BoltAddress* address, struct BoltConfig* config);

void BoltConnector_destroy(struct BoltConnector* connector);

struct BoltConnectionResult BoltConnector_acquire(struct BoltConnector* connector, enum BoltAccessMode mode);

void BoltConnector_release(struct BoltConnector* connector, struct BoltConnection* connection);

#endif //SEABOLT_ALL_CONNECTOR_H
