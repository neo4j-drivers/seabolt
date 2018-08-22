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



#include <cstring>
#include <cstdlib>
#include <integration.hpp>
#include "catch.hpp"

struct BoltAddress* bolt_get_address(const char* host, const char* port)
{
    struct BoltAddress* address = BoltAddress_create(host, port);
    int status = BoltAddress_resolve(address, NULL);
    REQUIRE(status==0);
    return address;
}

struct BoltConnection* bolt_open_b(enum BoltTransport transport, const char* host, const char* port)
{
    struct BoltAddress* address = bolt_get_address(host, port);
    struct BoltConnection* connection = BoltConnection_create();
    BoltConnection_open(connection, transport, address, NULL);
    BoltAddress_destroy(address);
    REQUIRE(connection->status==BOLT_CONNECTED);
    return connection;
}

struct BoltConnection* bolt_open_init_b(enum BoltTransport transport, const char* host, const char* port,
        const char* user_agent, const struct BoltValue* auth_token)
{
    struct BoltConnection* connection = bolt_open_b(transport, host, port);
    BoltConnection_init(connection, user_agent, auth_token);
    REQUIRE(connection->status==BOLT_READY);
    return connection;
}

struct BoltConnection* bolt_open_init_default()
{
    struct BoltValue* auth_token = BoltAuth_basic(BOLT_USER, BOLT_PASSWORD, NULL);
    struct BoltConnection* connection = bolt_open_init_b(BOLT_SECURE_SOCKET, BOLT_IPV6_HOST, BOLT_PORT, BOLT_USER_AGENT, auth_token);
    BoltValue_destroy(auth_token);
    return connection;
}

void bolt_close_and_destroy_b(struct BoltConnection* connection)
{
    BoltConnection_close(connection);
    BoltConnection_destroy(connection);
}
