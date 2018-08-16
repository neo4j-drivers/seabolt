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


#include "integration.hpp"
#include "catch.hpp"

SCENARIO("Test using a pooled connection", "[integration][ipv6][secure][pooling]")
{
    GIVEN("a new connection pool") {
        const auto auth_token = BoltAuth_basic(BOLT_USER, BOLT_PASSWORD, NULL);
        struct BoltConfig config { BOLT_DIRECT, BOLT_SECURE_SOCKET, BOLT_USER_AGENT, auth_token, NULL, 10 };
        struct BoltConnectionPool* pool = BoltConnectionPool_create(&BOLT_IPV6_ADDRESS, &config);
        WHEN("a connection is acquired") {
            struct BoltConnectionResult result = BoltConnectionPool_acquire(pool);
            THEN("the connection should be connected") {
                REQUIRE(result.connection_status==BOLT_READY);
                REQUIRE(result.connection_error==BOLT_SUCCESS);
                REQUIRE(result.connection_error_ctx==nullptr);
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
        struct BoltConfig config { BOLT_DIRECT, BOLT_SECURE_SOCKET, BOLT_USER_AGENT, auth_token, NULL, 1 };
        struct BoltConnectionPool* pool = BoltConnectionPool_create(&BOLT_IPV6_ADDRESS, &config);
        WHEN("a connection is acquired, released and acquired again") {
            struct BoltConnectionResult result1 = BoltConnectionPool_acquire(pool);
            BoltConnectionPool_release(pool, result1.connection);
            struct BoltConnectionResult result2 = BoltConnectionPool_acquire(pool);
            THEN("the connection should be connected") {
                REQUIRE(result2.connection_status==BOLT_READY);
                REQUIRE(result2.connection_error==BOLT_SUCCESS);
                REQUIRE(result2.connection_error_ctx==nullptr);
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
        struct BoltConfig config { BOLT_DIRECT, BOLT_SECURE_SOCKET, BOLT_USER_AGENT, auth_token, NULL, 1 };
        struct BoltConnectionPool* pool = BoltConnectionPool_create(&BOLT_IPV6_ADDRESS, &config);
        WHEN("a connection is acquired, released and acquired again") {
            struct BoltConnectionResult result1 = BoltConnectionPool_acquire(pool);
            THEN("handle should include a valid connection") {
                REQUIRE(result1.connection_status==BOLT_READY);
                REQUIRE(result1.connection_error==BOLT_SUCCESS);
                REQUIRE(result1.connection_error_ctx==nullptr);
                REQUIRE(result1.connection!=nullptr);
            }
            const char* cypher = "RETURN 1";
            BoltConnection_cypher(result1.connection, cypher, strlen(cypher), 0);
            BoltConnection_load_run_request(result1.connection);
            BoltConnection_send(result1.connection);
            BoltConnectionPool_release(pool, result1.connection);
            struct BoltConnectionResult result2 = BoltConnectionPool_acquire(pool);
            THEN("handle should include a valid connection") {
                REQUIRE(result2.connection_status==BOLT_READY);
                REQUIRE(result2.connection_error==BOLT_SUCCESS);
                REQUIRE(result2.connection_error_ctx==nullptr);
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
        struct BoltConfig config { BOLT_DIRECT, BOLT_SECURE_SOCKET, BOLT_USER_AGENT, auth_token, NULL, 1 };
        struct BoltConnectionPool* pool = BoltConnectionPool_create(&BOLT_IPV6_ADDRESS, &config);
        WHEN("two connections are acquired in turn") {
            struct BoltConnectionResult result1 = BoltConnectionPool_acquire(pool);
            struct BoltConnectionResult result2 = BoltConnectionPool_acquire(pool);
            THEN("the first connection should be connected") {
                REQUIRE(result1.connection_status==BOLT_READY);
                REQUIRE(result1.connection_error==BOLT_SUCCESS);
                REQUIRE(result1.connection_error_ctx==nullptr);
                REQUIRE(result1.connection!=nullptr);
                REQUIRE(result1.connection->status==BOLT_READY);
            }
            AND_THEN("the second connection should be invalid") {
                REQUIRE(result2.connection_status==BOLT_DISCONNECTED);
                REQUIRE(result2.connection_error==BOLT_POOL_FULL);
                REQUIRE(result2.connection_error_ctx==nullptr);
                REQUIRE(result2.connection==nullptr);
            }
            BoltConnectionPool_release(pool, result2.connection);
            BoltConnectionPool_release(pool, result1.connection);
        }
        BoltConnectionPool_destroy(pool);
        BoltValue_destroy(auth_token);
    }
}
