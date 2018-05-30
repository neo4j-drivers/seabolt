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


#include "integration.hpp"
#include "catch.hpp"


SCENARIO("Test basic secure connection (IPv4)", "[integration][ipv4][secure]")
{
    GIVEN("a local server address")
    {
        struct BoltAddress * address = bolt_get_address(BOLT_IPV4_HOST, BOLT_PORT);
        WHEN("a secure connection is opened")
        {
            struct BoltConnection * connection = BoltConnection_create();
            BoltConnection_open(connection, BOLT_SECURE_SOCKET, address);
            THEN("the connection should be connected")
            {
                REQUIRE(connection->status == BOLT_CONNECTED);
            }
            BoltConnection_close(connection);
            BoltConnection_destroy(connection);
        }
        BoltAddress_destroy(address);
    }
}

SCENARIO("Test basic secure connection (IPv6)", "[integration][ipv6][secure]")
{
    GIVEN("a local server address")
    {
        struct BoltAddress * address = bolt_get_address(BOLT_IPV6_HOST, BOLT_PORT);
        WHEN("a secure connection is opened")
        {
            struct BoltConnection * connection = BoltConnection_create();
            BoltConnection_open(connection, BOLT_SECURE_SOCKET, address);
            THEN("the connection should be connected")
            {
                REQUIRE(connection->status == BOLT_CONNECTED);
            }
            BoltConnection_close(connection);
            BoltConnection_destroy(connection);
        }
        BoltAddress_destroy(address);
    }
}

SCENARIO("Test basic insecure connection (IPv4)", "[integration][ipv4][insecure]")
{
    GIVEN("a local server address")
    {
        struct BoltAddress * address = bolt_get_address(BOLT_IPV4_HOST, BOLT_PORT);
        WHEN("an insecure connection is opened")
        {
            struct BoltConnection * connection = BoltConnection_create();
            BoltConnection_open(connection, BOLT_SOCKET, address);
            THEN("the connection should be connected")
            {
                REQUIRE(connection->status == BOLT_CONNECTED);
            }
            BoltConnection_close(connection);
            BoltConnection_destroy(connection);
        }
        BoltAddress_destroy(address);
    }
}

SCENARIO("Test basic insecure connection (IPv6)", "[integration][ipv6][insecure]")
{
    GIVEN("a local server address")
    {
        struct BoltAddress * address = bolt_get_address(BOLT_IPV6_HOST, BOLT_PORT);
        WHEN("an insecure connection is opened")
        {
            struct BoltConnection * connection = BoltConnection_create();
            BoltConnection_open(connection, BOLT_SOCKET, address);
            THEN("the connection should be connected")
            {
                REQUIRE(connection->status == BOLT_CONNECTED);
            }
            BoltConnection_close(connection);
            BoltConnection_destroy(connection);
        }
        BoltAddress_destroy(address);
    }
}

SCENARIO("Test secure connection to dead port", "[integration][ipv6][secure]")
{
    GIVEN("a local server address")
    {
        struct BoltAddress * address = bolt_get_address(BOLT_IPV6_HOST, "9999");
        WHEN("a secure connection attempt is made")
        {
            struct BoltConnection * connection = BoltConnection_create();
            BoltConnection_open(connection, BOLT_SECURE_SOCKET, address);
            THEN("a DEFUNCT connection should be returned")
            {
                REQUIRE(connection->status == BOLT_DEFUNCT);
            }
            BoltConnection_close(connection);
            BoltConnection_destroy(connection);
        }
        BoltAddress_destroy(address);
    }
}

SCENARIO("Test insecure connection to dead port", "[integration][ipv6][insecure]")
{
    GIVEN("a local server address")
    {
        struct BoltAddress * address = bolt_get_address(BOLT_IPV6_HOST, "9999");
        WHEN("an insecure connection attempt is made")
        {
            struct BoltConnection * connection = BoltConnection_create();
            BoltConnection_open(connection, BOLT_SOCKET, address);
            THEN("a DEFUNCT connection should be returned")
            {
                REQUIRE(connection->status == BOLT_DEFUNCT);
            }
            BoltConnection_close(connection);
            BoltConnection_destroy(connection);
        }
        BoltAddress_destroy(address);
    }
}

SCENARIO("Test connection reuse after graceful shutdown", "[integration][ipv6][secure]")
{
    GIVEN("a local server address")
    {
        struct BoltAddress * address = bolt_get_address(BOLT_IPV6_HOST, BOLT_PORT);
        WHEN("a secure connection is opened")
        {
            struct BoltConnection * connection = BoltConnection_create();
            BoltConnection_open(connection, BOLT_SECURE_SOCKET, address);
            THEN("the connection should be connected")
            {
                REQUIRE(connection->status == BOLT_CONNECTED);
            }
            BoltConnection_close(connection);
            THEN("the connection should be disconnected")
            {
                REQUIRE(connection->status == BOLT_DISCONNECTED);
            }
            BoltConnection_open(connection, BOLT_SECURE_SOCKET, address);
            THEN("the connection should be connected")
            {
                REQUIRE(connection->status == BOLT_CONNECTED);
            }
            BoltConnection_close(connection);
            BoltConnection_destroy(connection);
        }
        BoltAddress_destroy(address);
    }
}

SCENARIO("Test connection reuse after graceless shutdown", "[integration][ipv6][secure]")
{
    GIVEN("a local server address")
    {
        struct BoltAddress * address = bolt_get_address(BOLT_IPV6_HOST, BOLT_PORT);
        WHEN("a secure connection is opened")
        {
            struct BoltConnection * connection = BoltConnection_create();
            BoltConnection_open(connection, BOLT_SECURE_SOCKET, address);
            THEN("the connection should be connected")
            {
                REQUIRE(connection->status == BOLT_CONNECTED);
            }
            connection->status = BOLT_DEFUNCT;
            BoltConnection_open(connection, BOLT_SECURE_SOCKET, address);
            THEN("the connection should be connected")
            {
                REQUIRE(connection->status == BOLT_CONNECTED);
            }
            BoltConnection_close(connection);
            BoltConnection_destroy(connection);
        }
        BoltAddress_destroy(address);
    }
}

SCENARIO("Test init with valid credentials", "[integration][ipv6][secure]")
{
    GIVEN("an open connection")
    {
        struct BoltConnection * connection = bolt_open_b(BOLT_SECURE_SOCKET, BOLT_IPV6_HOST, BOLT_PORT);
        WHEN("successfully initialised")
        {
            int rv = BoltConnection_init(connection, &BOLT_PROFILE);
            THEN("return value should be 0")
            {
                REQUIRE(rv == 0);
            }
            THEN("status should change to READY")
            {
                REQUIRE(connection->status == BOLT_READY);
            }
        }
        BoltConnection_close(connection);
        BoltConnection_destroy(connection);
    }
}

SCENARIO("Test init with invalid credentials", "[integration][ipv6][secure]")
{
    GIVEN("an open connection")
    {
        struct BoltConnection * connection = bolt_open_b(BOLT_SECURE_SOCKET, BOLT_IPV6_HOST, BOLT_PORT);
        WHEN("unsuccessfully initialised")
        {
            REQUIRE(strcmp(BOLT_PASSWORD, "X") != 0);
            int rv = BoltConnection_init(connection, &BOLT_PROFILE);
            THEN("return value should not be 0")
            {
                REQUIRE(rv != 0);
            }
            THEN("status should change to DEFUNCT")
            {
                REQUIRE(connection->status == BOLT_DEFUNCT);
            }
        }
        BoltConnection_close(connection);
        BoltConnection_destroy(connection);
    }
}

SCENARIO("Test execution of simple Cypher statement", "[integration][ipv6][secure]")
{
    GIVEN("an open and initialised connection")
    {
        struct BoltConnection * connection = bolt_open_init_b(BOLT_SECURE_SOCKET, BOLT_IPV6_HOST, BOLT_PORT, &BOLT_PROFILE);
        WHEN("successfully executed Cypher")
        {
            const char * cypher = "RETURN 1";
            BoltConnection_cypher(connection, cypher, strlen(cypher), 0);
            BoltConnection_load_run_request(connection);
            bolt_request_t run = BoltConnection_last_request(connection);
            BoltConnection_load_pull_request(connection, -1);
            bolt_request_t pull = BoltConnection_last_request(connection);
            BoltConnection_send(connection);
            int records = BoltConnection_fetch_summary(connection, run);
            REQUIRE(records == 0);
            records = BoltConnection_fetch_summary(connection, pull);
            REQUIRE(records == 1);
        }
        BoltConnection_close(connection);
    }
}

SCENARIO("Test field names returned from Cypher execution", "[integration][ipv6][secure]")
{
    GIVEN("an open and initialised connection")
    {
        struct BoltConnection * connection = bolt_open_init_b(BOLT_SECURE_SOCKET, BOLT_IPV6_HOST, BOLT_PORT, &BOLT_PROFILE);
        WHEN("successfully executed Cypher")
        {
            const char * cypher = "RETURN 1 AS first, true AS second, 3.14 AS third";
            BoltConnection_cypher(connection, cypher, strlen(cypher), 0);
            BoltConnection_load_run_request(connection);
            bolt_request_t run = BoltConnection_last_request(connection);
            BoltConnection_load_pull_request(connection, -1);
            BoltConnection_send(connection);
            bolt_request_t last = BoltConnection_last_request(connection);
            BoltConnection_fetch_summary(connection, run);
            int n_fields = BoltConnection_result_n_fields(connection);
            REQUIRE(n_fields == 3);
            const char * field_name = BoltConnection_result_field_name(connection, 0);
            int field_name_size = BoltConnection_result_field_name_size(connection, 0);
            REQUIRE(strncmp(field_name, "first", field_name_size) == 0);
            field_name = BoltConnection_result_field_name(connection, 1);
            field_name_size = BoltConnection_result_field_name_size(connection, 1);
            REQUIRE(strncmp(field_name, "second", field_name_size) == 0);
            field_name = BoltConnection_result_field_name(connection, 2);
            field_name_size = BoltConnection_result_field_name_size(connection, 2);
            REQUIRE(strncmp(field_name, "third", field_name_size) == 0);
            REQUIRE(field_name_size == 5);
            BoltConnection_fetch_summary(connection, last);
        }
        BoltConnection_close(connection);
    }
}

SCENARIO("Test parameterised Cypher statements", "[integration][ipv6][secure]")
{
    GIVEN("an open and initialised connection")
    {
        struct BoltConnection * connection = bolt_open_init_b(BOLT_SECURE_SOCKET, BOLT_IPV6_HOST, BOLT_PORT, &BOLT_PROFILE);
        WHEN("successfully executed Cypher")
        {
            const char * cypher = "RETURN $x";
            BoltConnection_cypher(connection, cypher, strlen(cypher), 1);
            BoltValue * x = BoltConnection_cypher_parameter(connection, 0, "x", 1);
            BoltValue_format_as_Integer(x, 42);
            BoltConnection_load_run_request(connection);
            bolt_request_t run = BoltConnection_last_request(connection);
            BoltConnection_load_pull_request(connection, -1);
            bolt_request_t pull = BoltConnection_last_request(connection);
            BoltConnection_send(connection);
            int records = BoltConnection_fetch_summary(connection, run);
            REQUIRE(records == 0);
            struct BoltValue * last_received = BoltConnection_data(connection);
            REQUIRE(BoltConnection_summary_success(connection) == 1);
            while (BoltConnection_fetch(connection, pull))
            {
                REQUIRE(BoltValue_type(last_received) == BOLT_LIST);
                REQUIRE(last_received->size == 1);
                BoltValue * value = BoltList_value(last_received, 0);
                REQUIRE(BoltValue_type(value) == BOLT_INTEGER);
                REQUIRE(BoltInteger_get(value) == 42);
                records += 1;
            }
            REQUIRE(BoltConnection_summary_success(connection) == 1);
            REQUIRE(records == 1);
        }
        BoltConnection_close(connection);
    }
}

SCENARIO("Test execution of multiple Cypher statements transmitted together", "[integration][ipv6][secure]")
{
    GIVEN("an open and initialised connection")
    {
        struct BoltConnection * connection = bolt_open_init_b(BOLT_SECURE_SOCKET, BOLT_IPV6_HOST, BOLT_PORT, &BOLT_PROFILE);
        WHEN("successfully executed Cypher")
        {
            const char * cypher = "RETURN $x";
            BoltConnection_cypher(connection, cypher, strlen(cypher), 1);
            BoltValue * x = BoltConnection_cypher_parameter(connection, 0, "x", 1);
            BoltValue_format_as_Integer(x, 1);
            BoltConnection_load_run_request(connection);
            BoltConnection_load_discard_request(connection, -1);
            BoltValue_format_as_Integer(x, 2);
            BoltConnection_load_run_request(connection);
            BoltConnection_load_pull_request(connection, -1);
            BoltConnection_send(connection);
            unsigned long long last = BoltConnection_last_request(connection);
            int records = BoltConnection_fetch_summary(connection, last);
            REQUIRE(records == 1);
        }
        BoltConnection_close(connection);
    }
}

SCENARIO("Test transactions", "[integration][ipv6][secure]")
{
    GIVEN("an open and initialised connection")
    {
        struct BoltConnection * connection = bolt_open_init_b(BOLT_SECURE_SOCKET, BOLT_IPV6_HOST, BOLT_PORT, &BOLT_PROFILE);
        WHEN("successfully executed Cypher")
        {
            BoltConnection_load_begin_request(connection);
            bolt_request_t begin = BoltConnection_last_request(connection);

            const char * cypher = "RETURN 1";
            BoltConnection_cypher(connection, cypher, strlen(cypher), 0);
            BoltConnection_load_run_request(connection);
            bolt_request_t run = BoltConnection_last_request(connection);
            BoltConnection_load_pull_request(connection, -1);
            bolt_request_t pull = BoltConnection_last_request(connection);

            BoltConnection_load_commit_request(connection);
            bolt_request_t commit = BoltConnection_last_request(connection);

            BoltConnection_send(connection);
            bolt_request_t last = BoltConnection_last_request(connection);
            REQUIRE(last == commit);

            int records = BoltConnection_fetch_summary(connection, begin);
            REQUIRE(records == 0);
            REQUIRE(BoltConnection_summary_success(connection) == 1);

            records = BoltConnection_fetch_summary(connection, run);
            REQUIRE(records == 0);
            REQUIRE(BoltConnection_summary_success(connection) == 1);

            while (BoltConnection_fetch(connection, pull))
            {
                BoltValue * value = BoltConnection_data(connection);
                REQUIRE(BoltValue_type(value) == BOLT_INTEGER);
                REQUIRE(BoltInteger_get(value) == 1);
                records += 1;
            }
            REQUIRE(BoltConnection_summary_success(connection) == 1);
            REQUIRE(records == 1);

            records = BoltConnection_fetch_summary(connection, commit);
            REQUIRE(records == 0);
            REQUIRE(BoltConnection_summary_success(connection) == 1);

        }
        BoltConnection_close(connection);
    }
}
