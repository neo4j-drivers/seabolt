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
    GIVEN("a new connection pool") {
        const auto auth_token = BoltAuth_basic(BOLT_USER, BOLT_PASSWORD, NULL);
        struct BoltConnectionPool* pool = BoltConnectionPool_create(BOLT_SECURE_SOCKET, &BOLT_IPV6_ADDRESS,
                BOLT_USER_AGENT, auth_token, 10);
        WHEN("a connection is acquired") {
            struct BoltConnectionPoolAcquireResult result = BoltConnectionPool_acquire(pool, "test");
            THEN("the connection should be connected") {
                REQUIRE(result.status==POOL_NO_ERROR);
                REQUIRE(result.connection!=nullptr);
                REQUIRE(result.connection->status==BOLT_READY);
            }
            BoltConnectionPool_release(pool, result.connection);
        }
        BoltConnectionPool_destroy(pool);
        BoltValue_destroy(auth_token);
    }
}

SCENARIO("Test reusing a pooled connection", "[integration][ipv6][secure][pooling]")
{
    GIVEN("a new connection pool with one entry") {
        const auto auth_token = BoltAuth_basic(BOLT_USER, BOLT_PASSWORD, NULL);
        struct BoltConnectionPool* pool = BoltConnectionPool_create(BOLT_SECURE_SOCKET, &BOLT_IPV6_ADDRESS,
                BOLT_USER_AGENT, auth_token, 1);
        WHEN("a connection is acquired, released and acquired again") {
            struct BoltConnectionPoolAcquireResult result1 = BoltConnectionPool_acquire(pool, "test");
            BoltConnectionPool_release(pool, result1.connection);
            struct BoltConnectionPoolAcquireResult result2 = BoltConnectionPool_acquire(pool, "test");
            THEN("the connection should be connected") {
                REQUIRE(result2.status==POOL_NO_ERROR);
                REQUIRE(result2.connection!=nullptr);
                REQUIRE(result2.connection->status==BOLT_READY);
            }
            AND_THEN("the same connection should have been reused") {
                REQUIRE(result1.connection==result2.connection);
            }
            BoltConnectionPool_release(pool, result2.connection);
        }
        BoltConnectionPool_destroy(pool);
        BoltValue_destroy(auth_token);
    }
}

SCENARIO("Test reusing a pooled connection that was abandoned", "[integration][ipv6][secure][pooling]")
{
    GIVEN("a new connection pool with one entry") {
        const auto auth_token = BoltAuth_basic(BOLT_USER, BOLT_PASSWORD, NULL);
        struct BoltConnectionPool* pool = BoltConnectionPool_create(BOLT_SECURE_SOCKET, &BOLT_IPV6_ADDRESS,
                BOLT_USER_AGENT, auth_token, 1);
        WHEN("a connection is acquired, released and acquired again") {
            struct BoltConnectionPoolAcquireResult result1 = BoltConnectionPool_acquire(pool, "test");
            THEN("handle should include a valid connection") {
                REQUIRE(result1.status==POOL_NO_ERROR);
                REQUIRE(result1.connection!=nullptr);
            }
            const char* cypher = "RETURN 1";
            BoltConnection_cypher(result1.connection, cypher, strlen(cypher), 0);
            BoltConnection_load_run_request(result1.connection);
            BoltConnection_send(result1.connection);
            BoltConnectionPool_release(pool, result1.connection);
            struct BoltConnectionPoolAcquireResult result2 = BoltConnectionPool_acquire(pool, "test");
            THEN("handle should include a valid connection") {
                REQUIRE(result2.status==POOL_NO_ERROR);
                REQUIRE(result2.connection!=nullptr);
            }
            THEN("the connection should be connected") {
                REQUIRE(result2.connection->status==BOLT_READY);
            }
            AND_THEN("the same connection should have been reused") {
                REQUIRE(result1.connection==result2.connection);
            }
            BoltConnectionPool_release(pool, result2.connection);
        }
        BoltConnectionPool_destroy(pool);
        BoltValue_destroy(auth_token);
    }
}

SCENARIO("Test running out of connections", "[integration][ipv6][secure][pooling]")
{
    GIVEN("a new connection pool with one entry") {
        const auto auth_token = BoltAuth_basic(BOLT_USER, BOLT_PASSWORD, NULL);
        struct BoltConnectionPool* pool = BoltConnectionPool_create(BOLT_SECURE_SOCKET, &BOLT_IPV6_ADDRESS,
                BOLT_USER_AGENT, auth_token, 1);
        WHEN("two connections are acquired in turn") {
            struct BoltConnectionPoolAcquireResult result1 = BoltConnectionPool_acquire(pool, "test");
            struct BoltConnectionPoolAcquireResult result2 = BoltConnectionPool_acquire(pool, "test");
            THEN("the first connection should be connected") {
                REQUIRE(result1.status==POOL_NO_ERROR);
                REQUIRE(result1.connection!=nullptr);
                REQUIRE(result1.connection->status==BOLT_READY);
            }
            AND_THEN("the second connection should be invalid") {
                REQUIRE(result2.status==POOL_FULL);
                REQUIRE(result2.connection==nullptr);
            }
            BoltConnectionPool_release(pool, result2.connection);
            BoltConnectionPool_release(pool, result1.connection);
        }
        BoltConnectionPool_destroy(pool);
        BoltValue_destroy(auth_token);
    }
}
