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


SCENARIO("Test using a pooled connection", "[integration][ipv6][secure][pooling]")
{
    GIVEN("a new connection pool")
    {
        struct BoltAddress address { BOLT_IPV6_HOST, BOLT_PORT };
        struct BoltUserProfile profile { BOLT_AUTH_BASIC, BOLT_USER, BOLT_PASSWORD, BOLT_USER_AGENT };
        struct BoltConnectionPool * pool = BoltConnectionPool_create(BOLT_SECURE_SOCKET, &address, &profile, 10);
        WHEN("a connection is acquired")
        {
            struct BoltConnection * connection = BoltConnectionPool_acquire(pool, "test");
            THEN("the connection should be connected")
            {
                REQUIRE(connection->status == BOLT_READY);
            }
            BoltConnectionPool_release(pool, connection);
        }
        BoltConnectionPool_destroy(pool);
    }
}

SCENARIO("Test reusing a pooled connection", "[integration][ipv6][secure][pooling]")
{
    GIVEN("a new connection pool with one entry")
    {
        struct BoltAddress address { BOLT_IPV6_HOST, BOLT_PORT };
        struct BoltUserProfile profile { BOLT_AUTH_BASIC, BOLT_USER, BOLT_PASSWORD, BOLT_USER_AGENT };
        struct BoltConnectionPool * pool = BoltConnectionPool_create(BOLT_SECURE_SOCKET, &address, &profile, 1);
        WHEN("a connection is acquired, released and acquired again")
        {
            struct BoltConnection * connection1 = BoltConnectionPool_acquire(pool, "test");
            BoltConnectionPool_release(pool, connection1);
            struct BoltConnection * connection2 = BoltConnectionPool_acquire(pool, "test");
            THEN("the connection should be connected")
            {
                REQUIRE(connection2->status == BOLT_READY);
            }
            AND_THEN("the same connection should have been reused")
            {
                REQUIRE(connection1 == connection2);
            }
            BoltConnectionPool_release(pool, connection2);
        }
        BoltConnectionPool_destroy(pool);
    }
}

SCENARIO("Test reusing a pooled connection that was abandoned", "[integration][ipv6][secure][pooling]")
{
    GIVEN("a new connection pool with one entry")
    {
        struct BoltAddress address { BOLT_IPV6_HOST, BOLT_PORT };
        struct BoltUserProfile profile { BOLT_AUTH_BASIC, BOLT_USER, BOLT_PASSWORD, BOLT_USER_AGENT };
        struct BoltConnectionPool * pool = BoltConnectionPool_create(BOLT_SECURE_SOCKET, &address, &profile, 1);
        WHEN("a connection is acquired, released and acquired again")
        {
            struct BoltConnection * connection1 = BoltConnectionPool_acquire(pool, "test");
            const char * cypher = "RETURN 1";
            BoltConnection_cypher(connection1, cypher, strlen(cypher), 0);
            BoltConnection_load_run_request(connection1);
            BoltConnection_send(connection1);
            BoltConnectionPool_release(pool, connection1);
            struct BoltConnection * connection2 = BoltConnectionPool_acquire(pool, "test");
            THEN("the connection should be connected")
            {
                REQUIRE(connection2->status == BOLT_READY);
            }
            AND_THEN("the same connection should have been reused")
            {
                REQUIRE(connection1 == connection2);
            }
            BoltConnectionPool_release(pool, connection2);
        }
        BoltConnectionPool_destroy(pool);
    }
}

SCENARIO("Test running out of connections", "[integration][ipv6][secure][pooling]")
{
    GIVEN("a new connection pool with one entry")
    {
        struct BoltAddress address { BOLT_IPV6_HOST, BOLT_PORT };
        struct BoltUserProfile profile { BOLT_AUTH_BASIC, BOLT_USER, BOLT_PASSWORD, BOLT_USER_AGENT };
        struct BoltConnectionPool * pool = BoltConnectionPool_create(BOLT_SECURE_SOCKET, &address, &profile, 1);
        WHEN("two connections are acquired in turn")
        {
            struct BoltConnection * connection1 = BoltConnectionPool_acquire(pool, "test");
            struct BoltConnection * connection2 = BoltConnectionPool_acquire(pool, "test");
            THEN("the first connection should be connected")
            {
                REQUIRE(connection1->status == BOLT_READY);
            }
            AND_THEN("the second connection should be invalid")
            {
                REQUIRE(connection2 == NULL);
            }
            BoltConnectionPool_release(pool, connection2);
            BoltConnectionPool_release(pool, connection1);
        }
        BoltConnectionPool_destroy(pool);
    }
}
