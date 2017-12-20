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

const char* BOLT_IPV4_HOST = getenv_or_default("BOLT_IPV4_HOST", "127.0.0.1");
const char* BOLT_IPV6_HOST = getenv_or_default("BOLT_IPV6_HOST", "::1");
const char* BOLT_PORT = getenv_or_default("BOLT_PORT", "7687");
const char* BOLT_USER = getenv_or_default("BOLT_USER", "neo4j");
const char* BOLT_PASSWORD = getenv_or_default("BOLT_PASSWORD", "password");


SCENARIO("Test basic secure connection (IPv4)", "[integration][ipv4][secure]")
{
    GIVEN("a local server address")
    {
        struct addrinfo* address;
        int gai_status = getaddrinfo(BOLT_IPV4_HOST, BOLT_PORT, nullptr, &address);
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

SCENARIO("Test basic secure connection (IPv6)", "[integration][ipv6][secure]")
{
    GIVEN("a local server address")
    {
        struct addrinfo* address;
        int gai_status = getaddrinfo(BOLT_IPV6_HOST, BOLT_PORT, nullptr, &address);
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

SCENARIO("Test basic insecure connection (IPv4)", "[integration][ipv4][insecure]")
{
    GIVEN("a local server address")
    {
        struct addrinfo* address;
        int gai_status = getaddrinfo(BOLT_IPV4_HOST, BOLT_PORT, nullptr, &address);
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

SCENARIO("Test basic insecure connection (IPv6)", "[integration][ipv6][insecure]")
{
    GIVEN("a local server address")
    {
        struct addrinfo* address;
        int gai_status = getaddrinfo(BOLT_IPV6_HOST, BOLT_PORT, nullptr, &address);
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

SCENARIO("Test secure connection to dead port", "[integration][ipv6][secure]")
{
    GIVEN("a local server address")
    {
        struct addrinfo* address;
        int gai_status = getaddrinfo(BOLT_IPV6_HOST, "9999", nullptr, &address);
        REQUIRE(gai_status == 0);
        WHEN("a secure connection attempt is made")
        {
            struct BoltConnection* connection = BoltConnection_open_b(BOLT_SECURE_SOCKET, address);
            THEN("a DEFUNCT connection should be returned")
            {
                REQUIRE(connection->status == BOLT_DEFUNCT);
            }
            BoltConnection_close_b(connection);
        }
        freeaddrinfo(address);
    }
}

SCENARIO("Test insecure connection to dead port", "[integration][ipv6][insecure]")
{
    GIVEN("a local server address")
    {
        struct addrinfo* address;
        int gai_status = getaddrinfo(BOLT_IPV6_HOST, "9999", nullptr, &address);
        REQUIRE(gai_status == 0);
        WHEN("an insecure connection attempt is made")
        {
            struct BoltConnection* connection = BoltConnection_open_b(BOLT_INSECURE_SOCKET, address);
            THEN("a DEFUNCT connection should be returned")
            {
                REQUIRE(connection->status == BOLT_DEFUNCT);
            }
            BoltConnection_close_b(connection);
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

SCENARIO("Test init with valid credentials", "[integration][ipv6][secure]")
{
    GIVEN("an open connection")
    {
        struct BoltConnection* connection = _open_b(BOLT_SECURE_SOCKET, BOLT_IPV6_HOST, BOLT_PORT);
        WHEN("successfully initialised")
        {
            int rv = BoltConnection_init_b(connection, "seabolt/1.0.0a", BOLT_USER, BOLT_PASSWORD);
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

SCENARIO("Test init with invalid credentials", "[integration][ipv6][secure]")
{
    GIVEN("an open connection")
    {
        struct BoltConnection* connection = _open_b(BOLT_SECURE_SOCKET, BOLT_IPV6_HOST, BOLT_PORT);
        WHEN("unsuccessfully initialised")
        {
            REQUIRE(strcmp(BOLT_PASSWORD, "X") != 0);
            int rv = BoltConnection_init_b(connection, "seabolt/1.0.0a", BOLT_USER, "X");
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

struct BoltConnection* _open_and_init_b(enum BoltTransport transport, const char* host, const char* port,
                                    const char* user, const char* password)
{
    struct BoltConnection* connection = _open_b(transport, host, port);
    BoltConnection_init_b(connection, "seabolt/1.0.0a", user, password);
    REQUIRE(connection->status == BOLT_READY);
    return connection;
}

SCENARIO("Test execution of simple Cypher statement", "[integration][ipv6][secure]")
{
    GIVEN("an open and initialised connection")
    {
        struct BoltConnection* connection = _open_and_init_b(BOLT_SECURE_SOCKET, BOLT_IPV6_HOST, BOLT_PORT,
                                                             BOLT_USER, BOLT_PASSWORD);
        WHEN("successfully executed Cypher")
        {
            BoltConnection_set_statement(connection, "RETURN 1", 8);
            BoltConnection_resize_parameters(connection, 0);
            BoltConnection_load_run(connection);
            BoltConnection_load_pull(connection, -1);
            int requests = BoltConnection_transmit_b(connection);
            int responses = BoltConnection_receive_b(connection);
            REQUIRE(requests == 2);
            REQUIRE(responses == 2);
        }
        BoltConnection_close_b(connection);
    }
}

SCENARIO("Test parameterised Cypher statements", "[integration][ipv6][secure]")
{
    GIVEN("an open and initialised connection")
    {
        struct BoltConnection* connection = _open_and_init_b(BOLT_SECURE_SOCKET, BOLT_IPV6_HOST, BOLT_PORT,
                                                             BOLT_USER, BOLT_PASSWORD);
        WHEN("successfully executed Cypher")
        {
            BoltConnection_set_statement(connection, "RETURN $x", 9);
            BoltValue_to_Dictionary8(connection->cypher_parameters, 1);
            BoltDictionary8_set_key(connection->cypher_parameters, 0, "x", 1);
            BoltValue* x = BoltDictionary8_value(connection->cypher_parameters, 0);
            BoltValue_to_Int64(x, 42);
            BoltConnection_load_run(connection);
            BoltConnection_load_pull(connection, -1);
            int requests = BoltConnection_transmit_b(connection);
            REQUIRE(requests == 2);
            int records = BoltConnection_receive_summary_b(connection);
            REQUIRE(records == 0);
            REQUIRE(BoltValue_type(connection->received) == BOLT_SUMMARY);
            REQUIRE(BoltSummary_code(connection->received) == 0x70);
            while (BoltConnection_receive_value_b(connection))
            {
                REQUIRE(BoltValue_type(connection->received) == BOLT_LIST);
                REQUIRE(connection->received->size == 1);
                BoltValue* value = BoltList_value(connection->received, 0);
                REQUIRE(BoltValue_type(value) == BOLT_INT64);
                REQUIRE(BoltInt64_get(value) == 42);
                records += 1;
            }
            REQUIRE(BoltValue_type(connection->received) == BOLT_SUMMARY);
            REQUIRE(BoltSummary_code(connection->received) == 0x70);
            REQUIRE(records == 1);
        }
        BoltConnection_close_b(connection);
    }
}

SCENARIO("Test execution of multiple Cypher statements transmitted together", "[integration][ipv6][secure]")
{
    GIVEN("an open and initialised connection")
    {
        struct BoltConnection* connection = _open_and_init_b(BOLT_SECURE_SOCKET, BOLT_IPV6_HOST, BOLT_PORT,
                                                             BOLT_USER, BOLT_PASSWORD);
        WHEN("successfully executed Cypher")
        {
            BoltConnection_set_statement(connection, "RETURN $x", 9);
            BoltValue_to_Dictionary8(connection->cypher_parameters, 1);
            BoltDictionary8_set_key(connection->cypher_parameters, 0, "x", 1);
            BoltValue* x = BoltDictionary8_value(connection->cypher_parameters, 0);
            BoltValue_to_Int8(x, 1);
            BoltConnection_load_run(connection);
            BoltConnection_load_discard(connection, -1);
            BoltValue_to_Int8(x, 2);
            BoltConnection_load_run(connection);
            BoltConnection_load_pull(connection, -1);
            int requests = BoltConnection_transmit_b(connection);
            int responses = BoltConnection_receive_b(connection);
            REQUIRE(requests == 4);
            REQUIRE(responses == 4);
        }
        BoltConnection_close_b(connection);
    }
}
