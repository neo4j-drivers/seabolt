/*
 * Copyright (c) 2002-2018 "Neo Technology,"
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


#ifndef SEABOLT_TEST_INTEGRATION
#define SEABOLT_TEST_INTEGRATION


#include <cstring>
#include <cstdlib>

extern "C" {
    #include "bolt/direct.h"
    #include "bolt/logging.h"
    #include "bolt/pooling.h"
    #include "bolt/values.h"
}

#if USE_WINSOCK
#include <winsock2.h>
#endif

#define SETTING(name, default_value) ((getenv(name) == nullptr) ? (default_value) : getenv(name))

#define BOLT_IPV4_HOST  SETTING("BOLT_IPV4_HOST", "127.0.0.1")
#define BOLT_IPV6_HOST  SETTING("BOLT_IPV6_HOST", "::1")
#define BOLT_PORT       SETTING("BOLT_PORT", "7687")
#define BOLT_USER       SETTING("BOLT_USER", "neo4j")
#define BOLT_PASSWORD   SETTING("BOLT_PASSWORD", "password")
#define BOLT_USER_AGENT SETTING("BOLT_USER_AGENT", "seabolt/1.0.0a")

#define VERBOSE() BoltLog_set_file(stderr)

static const struct BoltUserProfile BOLT_PROFILE { BOLT_AUTH_BASIC, BOLT_USER, BOLT_PASSWORD, BOLT_USER_AGENT };


struct BoltAddress * bolt_get_address(const char * host, const char * port);

struct BoltConnection * bolt_open_b(enum BoltTransport transport, const char * host, const char * port);

struct BoltConnection * bolt_open_init_b(enum BoltTransport transport, const char * host, const char * port,
                                         const struct BoltUserProfile * profile);

void bolt_close_and_destroy_b(struct BoltConnection * connection);

#define NEW_BOLT_CONNECTION() bolt_open_init_b(BOLT_SECURE_SOCKET, BOLT_IPV6_HOST, BOLT_PORT, &BOLT_PROFILE)



#endif // SEABOLT_TEST_INTEGRATION
