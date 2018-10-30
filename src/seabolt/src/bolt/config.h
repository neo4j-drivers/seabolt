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
#ifndef SEABOLT_CONFIG_H
#define SEABOLT_CONFIG_H

#include "bolt-public.h"
#include "address-resolver.h"
#include "values.h"

typedef int32_t BoltMode;
#define BOLT_MODE_DIRECT    0
#define BOLT_MODE_ROUTING   1

typedef int32_t BoltTransport;
#define BOLT_TRANSPORT_PLAINTEXT    0
#define BOLT_TRANSPORT_ENCRYPTED    1

/**
 * Socket options
 */
typedef struct BoltSocketOptions BoltSocketOptions;

SEABOLT_EXPORT BoltSocketOptions* BoltSocketOptions_create();

SEABOLT_EXPORT void BoltSocketOptions_destroy(BoltSocketOptions* socket_options);

SEABOLT_EXPORT int32_t BoltSocketOptions_get_connect_timeout(BoltSocketOptions* socket_options);

SEABOLT_EXPORT int32_t BoltSocketOptions_set_connect_timeout(BoltSocketOptions* socket_options, int32_t connect_timeout);

SEABOLT_EXPORT int32_t BoltSocketOptions_get_keep_alive(BoltSocketOptions* socket_options);

SEABOLT_EXPORT int32_t BoltSocketOptions_set_keep_alive(BoltSocketOptions* socket_options, int32_t keep_alive);

/**
 * Trust options
 */
typedef struct BoltTrust BoltTrust;

SEABOLT_EXPORT BoltTrust* BoltTrust_create();

SEABOLT_EXPORT void BoltTrust_destroy(BoltTrust* trust);

SEABOLT_EXPORT const char* BoltTrust_get_certs(BoltTrust* trust, uint64_t* size);

SEABOLT_EXPORT int32_t BoltTrust_set_certs(BoltTrust* trust, const char* certs_pem, uint64_t certs_pem_size);

SEABOLT_EXPORT int32_t BoltTrust_get_skip_verify(BoltTrust* trust);

SEABOLT_EXPORT int32_t BoltTrust_set_skip_verify(BoltTrust* trust, int32_t skip_verify);

SEABOLT_EXPORT int32_t BoltTrust_get_skip_verify_hostname(BoltTrust* trust);

SEABOLT_EXPORT int32_t BoltTrust_set_skip_verify_hostname(BoltTrust* trust, int32_t skip_verify_hostname);

/**
 * Configuration options
 */
typedef struct BoltConfig BoltConfig;

SEABOLT_EXPORT BoltConfig* BoltConfig_create();

SEABOLT_EXPORT void BoltConfig_destroy(BoltConfig* config);

SEABOLT_EXPORT BoltMode BoltConfig_get_mode(BoltConfig* config);

SEABOLT_EXPORT int32_t BoltConfig_set_mode(BoltConfig* config, BoltMode mode);

SEABOLT_EXPORT BoltTransport BoltConfig_get_transport(BoltConfig* config);

SEABOLT_EXPORT int32_t BoltConfig_set_transport(BoltConfig* config, BoltTransport transport);

SEABOLT_EXPORT BoltTrust* BoltConfig_get_trust(BoltConfig* config);

SEABOLT_EXPORT int32_t BoltConfig_set_trust(BoltConfig* config, BoltTrust* trust);

SEABOLT_EXPORT const char* BoltConfig_get_user_agent(BoltConfig* config);

SEABOLT_EXPORT int32_t BoltConfig_set_user_agent(BoltConfig* config, const char* user_agent);

SEABOLT_EXPORT BoltValue* BoltConfig_get_routing_context(BoltConfig* config);

SEABOLT_EXPORT int32_t BoltConfig_set_routing_context(BoltConfig* config, BoltValue* routing_context);

SEABOLT_EXPORT BoltAddressResolver* BoltConfig_get_address_resolver(BoltConfig* config);

SEABOLT_EXPORT int32_t BoltConfig_set_address_resolver(BoltConfig* config, BoltAddressResolver* address_resolver);

SEABOLT_EXPORT BoltLog* BoltConfig_get_log(BoltConfig* config);

SEABOLT_EXPORT int32_t BoltConfig_set_log(BoltConfig* config, BoltLog* log);

SEABOLT_EXPORT int32_t BoltConfig_get_max_pool_size(BoltConfig* config);

SEABOLT_EXPORT int32_t BoltConfig_set_max_pool_size(BoltConfig* config, int32_t max_pool_size);

SEABOLT_EXPORT int32_t BoltConfig_get_max_connection_life_time(BoltConfig* config);

SEABOLT_EXPORT int32_t BoltConfig_set_max_connection_life_time(BoltConfig* config, int32_t max_connection_life_time);

SEABOLT_EXPORT int32_t BoltConfig_get_max_connection_acquisition_time(BoltConfig* config);

SEABOLT_EXPORT int32_t
BoltConfig_set_max_connection_acquisition_time(BoltConfig* config, int32_t max_connection_acquisition_time);

SEABOLT_EXPORT BoltSocketOptions* BoltConfig_get_socket_options(BoltConfig* config);

SEABOLT_EXPORT int32_t BoltConfig_set_socket_options(BoltConfig* config, BoltSocketOptions* socket_options);

#endif //SEABOLT_CONFIG_H
