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


#ifndef SEABOLT_TEST_INTEGRATION
#define SEABOLT_TEST_INTEGRATION

#include <cstring>
#include <cstdlib>
#include "bolt/bolt.h"

extern "C"
{
#include "bolt/address-private.h"
#include "bolt/address-set-private.h"
#include "bolt/config-private.h"
#include "bolt/connection-private.h"
#include "bolt/values-private.h"
#include "bolt/string-builder.h"
#include "bolt/direct-pool.h"
#include "bolt/v3.h"
}

#define SETTING(name, default_value) ((char*)((getenv(name) == nullptr) ? (default_value) : getenv(name)))

#define BOLT_IPV4_HOST  SETTING("BOLT_IPV4_HOST", "127.0.0.1")
#define BOLT_IPV6_HOST  SETTING("BOLT_IPV6_HOST", "::1")
#define BOLT_PORT       SETTING("BOLT_PORT", "7687")
#define BOLT_USER       SETTING("BOLT_USER", "neo4j")
#define BOLT_PASSWORD   SETTING("BOLT_PASSWORD", "password")
#define BOLT_USER_AGENT SETTING("BOLT_USER_AGENT", "seabolt/1.0.0a")

static struct BoltAddress BOLT_IPV6_ADDRESS{(char*) BOLT_IPV6_HOST, (char*) BOLT_PORT, 0, NULL, 0, NULL};

static struct BoltAddress BOLT_IPV4_ADDRESS{(char*) BOLT_IPV4_HOST, (char*) BOLT_PORT, 0, NULL, 0, NULL};

struct BoltAddress* bolt_get_address(const char* host, const char* port);

struct BoltConnection* bolt_open_b(BoltTransport transport, const char* host, const char* port);

struct BoltConnection* bolt_open_init_b(BoltTransport transport, const char* host, const char* port,
        const char* user_agent, const struct BoltValue* auth_token);

struct BoltConnection* bolt_open_init_default();

struct BoltValue* bolt_basic_auth(const char* username, const char* password);

void bolt_close_and_destroy_b(struct BoltConnection* connection);

#endif // SEABOLT_TEST_INTEGRATION
