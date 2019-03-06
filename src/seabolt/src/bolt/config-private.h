/*
 * Copyright (c) 2002-2019 "Neo4j,"
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
#ifndef SEABOLT_CONFIG_PRIVATE_H
#define SEABOLT_CONFIG_PRIVATE_H

#include "config.h"

/**
 * Use BOLT_SCHEME_CONNECTION to establish connections on-demand to a single server without
 * any connection pooling kicking in. The returned connection will behave as
 * BOLT_SCHEME_DIRECT.
 */
#define BOLT_SCHEME_DIRECT_UNPOOLED 2

struct BoltTrust {
    char* certs;
    uint64_t certs_len;
    int32_t skip_verify;
    int32_t skip_verify_hostname;
};

struct BoltSocketOptions {
    int32_t connect_timeout;
    int32_t recv_timeout;
    int32_t send_timeout;
    int32_t keep_alive;
};

struct BoltConfig {
    BoltScheme scheme;
    BoltTransport transport;
    struct BoltTrust* trust;
    char* user_agent;
    struct BoltValue* routing_context;
    struct BoltAddressResolver* address_resolver;
    struct BoltLog* log;
    int32_t max_pool_size;
    int32_t max_connection_life_time;
    int32_t max_connection_acquisition_time;
    struct BoltSocketOptions* socket_options;
};

BoltConfig* BoltConfig_clone(BoltConfig* config);

BoltTrust* BoltTrust_clone(BoltTrust* source);

BoltSocketOptions* BoltSocketOptions_clone(BoltSocketOptions* socket_options);

#endif //SEABOLT_CONFIG_PRIVATE_H
