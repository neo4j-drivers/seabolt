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

#include <memory.h>
#include <stdint.h>

#include "catch.hpp"

extern "C" {
    #include "bolt.h"
}


const char* getenv_or_default(const char* name, const char* default_value)
{
    const char* value = getenv(name);
    return (value == nullptr) ? default_value : value;
}

const char* BOLT_HOST = getenv_or_default("BOLT_HOST", "localhost");
const char* BOLT_PORT = getenv_or_default("BOLT_PORT", "7687");
const char* BOLT_USER = getenv_or_default("BOLT_USER", "neo4j");
const char* BOLT_PASSWORD = getenv_or_default("BOLT_PASSWORD", "password");


SCENARIO("Test basic secure connection", "[integration]")
{
    GIVEN("a local server address")
    {
        struct addrinfo* address;
        int gai_status = getaddrinfo(BOLT_HOST, BOLT_PORT, nullptr, &address);
        REQUIRE(gai_status == 0);
        WHEN("a secure connection is opened")
        {
            struct BoltConnection* connection = BoltConnection_open_b(BOLT_SECURE_SOCKET, address);
            THEN("the connection should be connected")
            {
                REQUIRE(connection->status == BOLT_CONNECTED);
            }
            BoltConnection_close_b(connection);
        }
        freeaddrinfo(address);
    }
}

SCENARIO("Test basic insecure connection", "[integration]")
{
    GIVEN("a local server address")
    {
        struct addrinfo* address;
        int gai_status = getaddrinfo(BOLT_HOST, BOLT_PORT, nullptr, &address);
        REQUIRE(gai_status == 0);
        WHEN("an insecure connection is opened")
        {
            struct BoltConnection* connection = BoltConnection_open_b(BOLT_INSECURE_SOCKET, address);
            THEN("the connection should be connected")
            {
                REQUIRE(connection->status == BOLT_CONNECTED);
            }
            BoltConnection_close_b(connection);
        }
        freeaddrinfo(address);
    }
}

SCENARIO("Test secure connection to dead port", "[integration]")
{
    GIVEN("a local server address")
    {
        struct addrinfo* address;
        int gai_status = getaddrinfo(BOLT_HOST, "9999", nullptr, &address);
        REQUIRE(gai_status == 0);
        WHEN("a secure connection attempt is made")
        {
            struct BoltConnection* connection = BoltConnection_open_b(BOLT_SECURE_SOCKET, address);
            THEN("a NULL value should be returned")
            {
                REQUIRE(connection == nullptr);
            }
        }
        freeaddrinfo(address);
    }
}

SCENARIO("Test insecure connection to dead port", "[integration]")
{
    GIVEN("a local server address")
    {
        struct addrinfo* address;
        int gai_status = getaddrinfo(BOLT_HOST, "9999", nullptr, &address);
        REQUIRE(gai_status == 0);
        WHEN("an insecure connection attempt is made")
        {
            struct BoltConnection* connection = BoltConnection_open_b(BOLT_INSECURE_SOCKET, address);
            THEN("a NULL value should be returned")
            {
                REQUIRE(connection == nullptr);
            }
        }
        freeaddrinfo(address);
    }
}

struct BoltConnection* _open_b(enum BoltTransport transport, const char* host, const char* port)
{
    struct addrinfo* address;
    int gai_status = getaddrinfo(host, port, nullptr, &address);
    REQUIRE(gai_status == 0);
    struct BoltConnection* connection = BoltConnection_open_b(transport, address);
    freeaddrinfo(address);
    REQUIRE(connection->status == BOLT_CONNECTED);
    return connection;
}

SCENARIO("Test init with valid credentials", "[integration]")
{
    GIVEN("an open connection")
    {
        struct BoltConnection* connection = _open_b(BOLT_SECURE_SOCKET, BOLT_HOST, BOLT_PORT);
        WHEN("successfully initialised")
        {
            int rv = BoltConnection_init_b(connection, BOLT_USER, BOLT_PASSWORD);
            THEN("return value should be 0")
            {
                REQUIRE(rv == 0);
            }
            THEN("status should change to READY")
            {
                REQUIRE(connection->status == BOLT_READY);
            }
        }
        BoltConnection_close_b(connection);
    }
}

SCENARIO("Test init with invalid credentials", "[integration]")
{
    GIVEN("an open connection")
    {
        struct BoltConnection* connection = _open_b(BOLT_SECURE_SOCKET, BOLT_HOST, BOLT_PORT);
        WHEN("unsuccessfully initialised")
        {
            REQUIRE(strcmp(BOLT_PASSWORD, "X") != 0);
            int rv = BoltConnection_init_b(connection, BOLT_USER, "X");
            THEN("return value should not be 0")
            {
                REQUIRE(rv != 0);
            }
            THEN("status should change to DEFUNCT")
            {
                REQUIRE(connection->status == BOLT_DEFUNCT);
            }
        }
        BoltConnection_close_b(connection);
    }
}
