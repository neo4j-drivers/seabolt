/*
 * Copyright (c) 2002-2019 "Neo4j,"
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
    BoltStatus* status = BoltStatus_create();
    GIVEN("a new connection pool") {
        const auto auth_token = BoltAuth_basic(BOLT_USER, BOLT_PASSWORD, NULL);
        struct BoltTrust trust{nullptr, 0, 1, 1};
        struct BoltConfig config{BOLT_SCHEME_DIRECT, BOLT_TRANSPORT_ENCRYPTED, &trust, BOLT_USER_AGENT, nullptr, nullptr,
                                 nullptr,
                                 10, 0, 0, NULL};
        struct BoltConnector* connector = BoltConnector_create(&BOLT_IPV6_ADDRESS, auth_token, &config);
        WHEN("a connection is acquired") {
            BoltConnection* connection = BoltConnector_acquire(connector, BOLT_ACCESS_MODE_READ, status);
            THEN("the connection should be connected") {
                REQUIRE(status->state==BOLT_CONNECTION_STATE_READY);
                REQUIRE(status->error==BOLT_SUCCESS);
                REQUIRE(status->error_ctx==nullptr);
                REQUIRE(connection!=nullptr);
                REQUIRE(connection->status->state==BOLT_CONNECTION_STATE_READY);
            }
            BoltConnector_release(connector, connection);
        }
        BoltConnector_destroy(connector);
        BoltValue_destroy(auth_token);
    }
    BoltStatus_destroy(status);
}

SCENARIO("Test reusing a pooled connection", "[integration][ipv6][secure][pooling]")
{
    BoltStatus* status1 = BoltStatus_create();
    BoltStatus* status2 = BoltStatus_create();
    GIVEN("a new connection pool with one entry") {
        const auto auth_token = BoltAuth_basic(BOLT_USER, BOLT_PASSWORD, NULL);
        struct BoltTrust trust{nullptr, 0, 1, 1};
        struct BoltConfig config{BOLT_SCHEME_DIRECT, BOLT_TRANSPORT_ENCRYPTED, &trust, BOLT_USER_AGENT, nullptr, nullptr,
                                 nullptr,
                                 1, 0, 0, NULL};
        struct BoltConnector* connector = BoltConnector_create(&BOLT_IPV6_ADDRESS, auth_token, &config);
        WHEN("a connection is acquired, released and acquired again") {
            BoltConnection* connection1 = BoltConnector_acquire(connector, BOLT_ACCESS_MODE_READ, status1);
            BoltConnector_release(connector, connection1);
            BoltConnection* connection2 = BoltConnector_acquire(connector, BOLT_ACCESS_MODE_READ, status2);
            THEN("the connection should be connected") {
                REQUIRE(status2->state==BOLT_CONNECTION_STATE_READY);
                REQUIRE(status2->error==BOLT_SUCCESS);
                REQUIRE(status2->error_ctx==nullptr);
                REQUIRE(connection2!=nullptr);
                REQUIRE(connection2->status->state==BOLT_CONNECTION_STATE_READY);
            }
            AND_THEN("the same connection should have been reused") {
                REQUIRE(connection1==connection2);
            }
            BoltConnector_release(connector, connection2);
        }
        BoltConnector_destroy(connector);
        BoltValue_destroy(auth_token);
    }
    BoltStatus_destroy(status1);
    BoltStatus_destroy(status2);
}

SCENARIO("Test reusing a pooled connection that was abandoned", "[integration][ipv6][secure][pooling]")
{
    BoltStatus* status1 = BoltStatus_create();
    BoltStatus* status2 = BoltStatus_create();
    GIVEN("a new connection pool with one entry") {
        const auto auth_token = BoltAuth_basic(BOLT_USER, BOLT_PASSWORD, NULL);
        struct BoltTrust trust{nullptr, 0, 1, 1};
        struct BoltConfig config{BOLT_SCHEME_DIRECT, BOLT_TRANSPORT_ENCRYPTED, &trust, BOLT_USER_AGENT, nullptr, nullptr,
                                 nullptr, 1, 0, 0, NULL};
        struct BoltConnector* connector = BoltConnector_create(&BOLT_IPV6_ADDRESS, auth_token, &config);
        WHEN("a connection is acquired, released and acquired again") {
            BoltConnection* connection1 = BoltConnector_acquire(connector, BOLT_ACCESS_MODE_READ, status1);
            THEN("handle should include a valid connection") {
                REQUIRE(status1->state==BOLT_CONNECTION_STATE_READY);
                REQUIRE(status1->error==BOLT_SUCCESS);
                REQUIRE(status1->error_ctx==nullptr);
                REQUIRE(connection1!=nullptr);
            }
            const char* cypher = "RETURN 1";
            BoltConnection_set_run_cypher(connection1, cypher, strlen(cypher), 0);
            BoltConnection_load_run_request(connection1);
            BoltConnection_send(connection1);
            BoltConnector_release(connector, connection1);
            BoltConnection* connection2 = BoltConnector_acquire(connector, BOLT_ACCESS_MODE_READ, status2);
            THEN("handle should include a valid connection") {
                REQUIRE(status2->state==BOLT_CONNECTION_STATE_READY);
                REQUIRE(status2->error==BOLT_SUCCESS);
                REQUIRE(status2->error_ctx==nullptr);
                REQUIRE(connection2!=nullptr);
            }
            THEN("the connection should be connected") {
                REQUIRE(connection2->status->state==BOLT_CONNECTION_STATE_READY);
            }
            AND_THEN("the same connection should have been reused") {
                REQUIRE(connection1==connection2);
            }
            BoltConnector_release(connector, connection2);
        }
        BoltConnector_destroy(connector);
        BoltValue_destroy(auth_token);
    }
    BoltStatus_destroy(status1);
    BoltStatus_destroy(status2);
}

SCENARIO("Test running out of connections", "[integration][ipv6][secure][pooling]")
{
    BoltStatus* status1 = BoltStatus_create();
    BoltStatus* status2 = BoltStatus_create();
    GIVEN("a new connection pool with one entry") {
        const auto auth_token = BoltAuth_basic(BOLT_USER, BOLT_PASSWORD, NULL);
        struct BoltTrust trust{nullptr, 0, 1, 1};
        struct BoltConfig config{BOLT_SCHEME_DIRECT, BOLT_TRANSPORT_ENCRYPTED, &trust, BOLT_USER_AGENT, nullptr, nullptr,
                                 nullptr,
                                 1, 0, 0, NULL};
        struct BoltConnector* connector = BoltConnector_create(&BOLT_IPV6_ADDRESS, auth_token, &config);
        WHEN("two connections are acquired in turn") {
            BoltConnection* connection1 = BoltConnector_acquire(connector, BOLT_ACCESS_MODE_READ, status1);
            BoltConnection* connection2 = BoltConnector_acquire(connector, BOLT_ACCESS_MODE_READ, status2);
            THEN("the first connection should be connected") {
                REQUIRE(status1->state==BOLT_CONNECTION_STATE_READY);
                REQUIRE(status1->error==BOLT_SUCCESS);
                REQUIRE(status1->error_ctx==nullptr);
                REQUIRE(connection1!=nullptr);
                REQUIRE(connection1->status->state==BOLT_CONNECTION_STATE_READY);
            }
            AND_THEN("the second connection should be invalid") {
                REQUIRE(status2->state==BOLT_CONNECTION_STATE_DISCONNECTED);
                REQUIRE(status2->error==BOLT_POOL_FULL);
                REQUIRE(status2->error_ctx==nullptr);
                REQUIRE(connection2==nullptr);
            }
            BoltConnector_release(connector, connection2);
            BoltConnector_release(connector, connection1);
        }
        BoltConnector_destroy(connector);
        BoltValue_destroy(auth_token);
    }
    BoltStatus_destroy(status1);
    BoltStatus_destroy(status2);
}
