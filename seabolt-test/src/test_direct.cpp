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

SCENARIO("Test basic secure connection (IPv4)", "[integration][ipv4][secure]")
{
    GIVEN("a local server address") {
        struct BoltAddress* address = bolt_get_address(BOLT_IPV4_HOST, BOLT_PORT);
        WHEN("a secure connection is opened") {
            struct BoltConnection* connection = BoltConnection_create();
            BoltConnection_open(connection, BOLT_SECURE_SOCKET, address);
            THEN("the connection should be connected") {
                REQUIRE(connection->status==BOLT_CONNECTED);
            }
            BoltConnection_close(connection);
            BoltConnection_destroy(connection);
        }
        BoltAddress_destroy(address);
    }
}

SCENARIO("Test basic secure connection (IPv6)", "[integration][ipv6][secure]")
{
    GIVEN("a local server address") {
        struct BoltAddress* address = bolt_get_address(BOLT_IPV6_HOST, BOLT_PORT);
        WHEN("a secure connection is opened") {
            struct BoltConnection* connection = BoltConnection_create();
            BoltConnection_open(connection, BOLT_SECURE_SOCKET, address);
            THEN("the connection should be connected") {
                REQUIRE(connection->status==BOLT_CONNECTED);
            }
            BoltConnection_close(connection);
            BoltConnection_destroy(connection);
        }
        BoltAddress_destroy(address);
    }
}

SCENARIO("Test basic insecure connection (IPv4)", "[integration][ipv4][insecure]")
{
    GIVEN("a local server address") {
        struct BoltAddress* address = bolt_get_address(BOLT_IPV4_HOST, BOLT_PORT);
        WHEN("an insecure connection is opened") {
            struct BoltConnection* connection = BoltConnection_create();
            BoltConnection_open(connection, BOLT_SOCKET, address);
            THEN("the connection should be connected") {
                REQUIRE(connection->status==BOLT_CONNECTED);
            }
            BoltConnection_close(connection);
            BoltConnection_destroy(connection);
        }
        BoltAddress_destroy(address);
    }
}

SCENARIO("Test basic insecure connection (IPv6)", "[integration][ipv6][insecure]")
{
    GIVEN("a local server address") {
        struct BoltAddress* address = bolt_get_address(BOLT_IPV6_HOST, BOLT_PORT);
        WHEN("an insecure connection is opened") {
            struct BoltConnection* connection = BoltConnection_create();
            BoltConnection_open(connection, BOLT_SOCKET, address);
            THEN("the connection should be connected") {
                REQUIRE(connection->status==BOLT_CONNECTED);
            }
            BoltConnection_close(connection);
            BoltConnection_destroy(connection);
        }
        BoltAddress_destroy(address);
    }
}

SCENARIO("Test secure connection to dead port", "[integration][ipv6][secure]")
{
    GIVEN("a local server address") {
        struct BoltAddress* address = bolt_get_address(BOLT_IPV6_HOST, "9999");
        WHEN("a secure connection attempt is made") {
            struct BoltConnection* connection = BoltConnection_create();
            BoltConnection_open(connection, BOLT_SECURE_SOCKET, address);
            THEN("a DEFUNCT connection should be returned") {
                REQUIRE(connection->status==BOLT_DEFUNCT);
            }
            BoltConnection_close(connection);
            BoltConnection_destroy(connection);
        }
        BoltAddress_destroy(address);
    }
}

SCENARIO("Test insecure connection to dead port", "[integration][ipv6][insecure]")
{
    GIVEN("a local server address") {
        struct BoltAddress* address = bolt_get_address(BOLT_IPV6_HOST, "9999");
        WHEN("an insecure connection attempt is made") {
            struct BoltConnection* connection = BoltConnection_create();
            BoltConnection_open(connection, BOLT_SOCKET, address);
            THEN("a DEFUNCT connection should be returned") {
                REQUIRE(connection->status==BOLT_DEFUNCT);
            }
            BoltConnection_close(connection);
            BoltConnection_destroy(connection);
        }
        BoltAddress_destroy(address);
    }
}

SCENARIO("Test connection reuse after graceful shutdown", "[integration][ipv6][secure]")
{
    GIVEN("a local server address") {
        struct BoltAddress* address = bolt_get_address(BOLT_IPV6_HOST, BOLT_PORT);
        WHEN("a secure connection is opened") {
            struct BoltConnection* connection = BoltConnection_create();
            BoltConnection_open(connection, BOLT_SECURE_SOCKET, address);
            THEN("the connection should be connected") {
                REQUIRE(connection->status==BOLT_CONNECTED);
            }
            BoltConnection_close(connection);
            THEN("the connection should be disconnected") {
                REQUIRE(connection->status==BOLT_DISCONNECTED);
            }
            BoltConnection_open(connection, BOLT_SECURE_SOCKET, address);
            THEN("the connection should be connected") {
                REQUIRE(connection->status==BOLT_CONNECTED);
            }
            BoltConnection_close(connection);
            BoltConnection_destroy(connection);
        }
        BoltAddress_destroy(address);
    }
}

SCENARIO("Test connection reuse after graceless shutdown", "[integration][ipv6][secure]")
{
    GIVEN("a local server address") {
        struct BoltAddress* address = bolt_get_address(BOLT_IPV6_HOST, BOLT_PORT);
        WHEN("a secure connection is opened") {
            struct BoltConnection* connection = BoltConnection_create();
            BoltConnection_open(connection, BOLT_SECURE_SOCKET, address);
            THEN("the connection should be connected") {
                REQUIRE(connection->status==BOLT_CONNECTED);
            }
            connection->status = BOLT_DEFUNCT;
            BoltConnection_open(connection, BOLT_SECURE_SOCKET, address);
            THEN("the connection should be connected") {
                REQUIRE(connection->status==BOLT_CONNECTED);
            }
            BoltConnection_close(connection);
            BoltConnection_destroy(connection);
        }
        BoltAddress_destroy(address);
    }
}

SCENARIO("Test init with valid credentials", "[integration][ipv6][secure]")
{
    GIVEN("an open connection") {
        const auto auth_token = BoltAuth_basic(BOLT_USER, BOLT_PASSWORD, NULL);
        struct BoltConnection* connection = bolt_open_b(BOLT_SECURE_SOCKET, BOLT_IPV6_HOST, BOLT_PORT);
        WHEN("successfully initialised") {
            int rv = BoltConnection_init(connection, BOLT_USER_AGENT, auth_token);
            THEN("return value should be 0") {
                REQUIRE(rv==0);
            }
            THEN("status should change to READY") {
                REQUIRE(connection->status==BOLT_READY);
            }
        }
        BoltConnection_close(connection);
        BoltConnection_destroy(connection);
        BoltValue_destroy(auth_token);
    }
}

SCENARIO("Test init with invalid credentials", "[integration][ipv6][secure]")
{
    GIVEN("an open connection") {
        struct BoltConnection* connection = bolt_open_b(BOLT_SECURE_SOCKET, BOLT_IPV6_HOST, BOLT_PORT);
        WHEN("unsuccessfully initialised") {
            REQUIRE(strcmp(BOLT_PASSWORD, "X")!=0);

            const auto auth_token = BoltAuth_basic(BOLT_USER, "X", NULL);

            int rv = BoltConnection_init(connection, BOLT_USER_AGENT, auth_token);
            THEN("return value should not be 0") {
                REQUIRE(rv!=0);
            }
            THEN("status should change to DEFUNCT") {
                REQUIRE(connection->status==BOLT_DEFUNCT);
            }

            BoltValue_destroy(auth_token);
        }
        BoltConnection_close(connection);
        BoltConnection_destroy(connection);
    }
}

SCENARIO("Test execution of simple Cypher statement", "[integration][ipv6][secure]")
{
    GIVEN("an open and initialised connection") {
        struct BoltConnection* connection = bolt_open_init_default();
        WHEN("successfully executed Cypher") {
            const char* cypher = "RETURN 1";
            BoltConnection_cypher(connection, cypher, strlen(cypher), 0);
            BoltConnection_load_run_request(connection);
            bolt_request_t run = BoltConnection_last_request(connection);
            BoltConnection_load_pull_request(connection, -1);
            bolt_request_t pull = BoltConnection_last_request(connection);
            BoltConnection_send(connection);
            int records = BoltConnection_fetch_summary(connection, run);
            REQUIRE(records==0);
            records = BoltConnection_fetch_summary(connection, pull);
            REQUIRE(records==1);
        }
        BoltConnection_close(connection);
    }
}

SCENARIO("Test field names returned from Cypher execution", "[integration][ipv6][secure]")
{
    GIVEN("an open and initialised connection") {
        struct BoltConnection* connection = bolt_open_init_default();
        WHEN("successfully executed Cypher") {
            const char* cypher = "RETURN 1 AS first, true AS second, 3.14 AS third";
            BoltConnection_cypher(connection, cypher, strlen(cypher), 0);
            BoltConnection_load_run_request(connection);
            bolt_request_t run = BoltConnection_last_request(connection);
            BoltConnection_load_pull_request(connection, -1);
            BoltConnection_send(connection);
            bolt_request_t last = BoltConnection_last_request(connection);
            BoltConnection_fetch_summary(connection, run);
            const struct BoltValue* fields = BoltConnection_fields(connection);
            REQUIRE(fields->size==3);
            struct BoltValue* field_name_value = BoltList_value(fields, 0);
            const char* field_name = BoltString_get(field_name_value);
            int field_name_size = field_name_value->size;
            REQUIRE(strncmp(field_name, "first", field_name_size)==0);
            field_name_value = BoltList_value(fields, 1);
            field_name = BoltString_get(field_name_value);
            field_name_size = field_name_value->size;
            REQUIRE(strncmp(field_name, "second", field_name_size)==0);
            field_name_value = BoltList_value(fields, 2);
            field_name = BoltString_get(field_name_value);
            field_name_size = field_name_value->size;
            REQUIRE(strncmp(field_name, "third", field_name_size)==0);
            REQUIRE(field_name_size==5);
            BoltConnection_fetch_summary(connection, last);
        }
        BoltConnection_close(connection);
    }
}

SCENARIO("Test parameterised Cypher statements", "[integration][ipv6][secure]")
{
    GIVEN("an open and initialised connection") {
        struct BoltConnection* connection = bolt_open_init_default();
        WHEN("successfully executed Cypher") {
            const char* cypher = "RETURN $x";
            BoltConnection_cypher(connection, cypher, strlen(cypher), 1);
            BoltValue* x = BoltConnection_cypher_parameter(connection, 0, "x", 1);
            BoltValue_format_as_Integer(x, 42);
            BoltConnection_load_run_request(connection);
            bolt_request_t run = BoltConnection_last_request(connection);
            BoltConnection_load_pull_request(connection, -1);
            bolt_request_t pull = BoltConnection_last_request(connection);
            BoltConnection_send(connection);
            int records = BoltConnection_fetch_summary(connection, run);
            REQUIRE(records==0);
            REQUIRE(BoltConnection_summary_success(connection)==1);
            while (BoltConnection_fetch(connection, pull)) {
                const struct BoltValue* field_values = BoltConnection_record_fields(connection);
                struct BoltValue* value = BoltList_value(field_values, 0);
                REQUIRE(BoltValue_type(value)==BOLT_INTEGER);
                REQUIRE(BoltInteger_get(value)==42);
                records += 1;
            }
            REQUIRE(BoltConnection_summary_success(connection)==1);
            REQUIRE(records==1);
        }
        BoltConnection_close(connection);
    }
}

SCENARIO("Test execution of multiple Cypher statements transmitted together", "[integration][ipv6][secure]")
{
    GIVEN("an open and initialised connection") {
        struct BoltConnection* connection = bolt_open_init_default();
        WHEN("successfully executed Cypher") {
            const char* cypher = "RETURN $x";
            BoltConnection_cypher(connection, cypher, strlen(cypher), 1);
            BoltValue* x = BoltConnection_cypher_parameter(connection, 0, "x", 1);
            BoltValue_format_as_Integer(x, 1);
            BoltConnection_load_run_request(connection);
            BoltConnection_load_discard_request(connection, -1);
            BoltValue_format_as_Integer(x, 2);
            BoltConnection_load_run_request(connection);
            BoltConnection_load_pull_request(connection, -1);
            BoltConnection_send(connection);
            unsigned long long last = BoltConnection_last_request(connection);
            int records = BoltConnection_fetch_summary(connection, last);
            REQUIRE(records==1);
        }
        BoltConnection_close(connection);
    }
}

SCENARIO("Test transactions", "[integration][ipv6][secure]")
{
    GIVEN("an open and initialised connection") {
        struct BoltConnection* connection = bolt_open_init_default();
        WHEN("successfully executed Cypher") {
            BoltConnection_load_begin_request(connection);
            bolt_request_t begin = BoltConnection_last_request(connection);

            const char* cypher = "RETURN 1";
            BoltConnection_cypher(connection, cypher, strlen(cypher), 0);
            BoltConnection_load_run_request(connection);
            bolt_request_t run = BoltConnection_last_request(connection);
            BoltConnection_load_pull_request(connection, -1);
            bolt_request_t pull = BoltConnection_last_request(connection);

            BoltConnection_load_commit_request(connection);
            bolt_request_t commit = BoltConnection_last_request(connection);

            BoltConnection_send(connection);
            bolt_request_t last = BoltConnection_last_request(connection);
            REQUIRE(last==commit);

            int records = BoltConnection_fetch_summary(connection, begin);
            REQUIRE(records==0);
            REQUIRE(BoltConnection_summary_success(connection)==1);

            records = BoltConnection_fetch_summary(connection, run);
            REQUIRE(records==0);
            REQUIRE(BoltConnection_summary_success(connection)==1);

            while (BoltConnection_fetch(connection, pull)) {
                const struct BoltValue* field_values = BoltConnection_record_fields(connection);
                struct BoltValue* value = BoltList_value(field_values, 0);
                REQUIRE(BoltValue_type(value)==BOLT_INTEGER);
                REQUIRE(BoltInteger_get(value)==1);
                records += 1;
            }
            REQUIRE(BoltConnection_summary_success(connection)==1);
            REQUIRE(records==1);

            records = BoltConnection_fetch_summary(connection, commit);
            REQUIRE(records==0);
            REQUIRE(BoltConnection_summary_success(connection)==1);

        }
        BoltConnection_close(connection);
    }
}

SCENARIO("Test FAILURE", "[integration][ipv6][secure]")
{
    GIVEN("an open and initialised connection") {
        auto connection = bolt_open_init_default();
        WHEN("an invalid cypher statement is sent") {
            const auto cypher = "some invalid statement";
            BoltConnection_cypher(connection, cypher, strlen(cypher), 0);
            BoltConnection_load_run_request(connection);
            const auto run = BoltConnection_last_request(connection);
            BoltConnection_load_pull_request(connection, -1);
            const auto pull = BoltConnection_last_request(connection);

            BoltConnection_send(connection);
            THEN("connector should be in FAILED state") {
                const auto records = BoltConnection_fetch_summary(connection, run);
                REQUIRE(records==0);
                REQUIRE(BoltConnection_summary_success(connection)==0);
                REQUIRE(connection->status==BOLT_FAILED);
                REQUIRE(connection->error==BOLT_SERVER_FAILURE);

                auto failure_data = BoltConnection_failure(connection);
                REQUIRE(failure_data!=nullptr);

                struct BoltValue* code = BoltDictionary_value_by_key(failure_data, "code", strlen("code"));
                struct BoltValue* message = BoltDictionary_value_by_key(failure_data, "message", strlen("message"));
                REQUIRE(code!=nullptr);
                REQUIRE(BoltValue_type(code)==BOLT_STRING);
                REQUIRE(BoltString_equals(code, "Neo.ClientError.Statement.SyntaxError"));
                REQUIRE(message!=nullptr);
                REQUIRE(BoltValue_type(message)==BOLT_STRING);
            }

            THEN("already sent requests should be IGNORED after FAILURE") {
                const auto records = BoltConnection_fetch_summary(connection, pull);

                REQUIRE(records==0);
                REQUIRE(BoltConnection_summary_success(connection)==0);
                REQUIRE(connection->status==BOLT_FAILED);
                REQUIRE(connection->error==BOLT_SERVER_FAILURE);

                struct BoltValue* failure_data = BoltConnection_failure(connection);
                REQUIRE(failure_data!=nullptr);

                struct BoltValue* code = BoltDictionary_value_by_key(failure_data, "code", strlen("code"));
                struct BoltValue* message = BoltDictionary_value_by_key(failure_data, "message", strlen("message"));
                REQUIRE(code!=nullptr);
                REQUIRE(BoltValue_type(code)==BOLT_STRING);
                REQUIRE(BoltString_equals(code, "Neo.ClientError.Statement.SyntaxError"));
                REQUIRE(message!=nullptr);
                REQUIRE(BoltValue_type(message)==BOLT_STRING);
            }

            THEN("upcoming requests should be IGNORED after FAILURE") {
                const auto cypher1 = "RETURN 1";
                BoltConnection_cypher(connection, cypher1, strlen(cypher1), 0);
                BoltConnection_load_run_request(connection);
                const auto run1 = BoltConnection_last_request(connection);

                BoltConnection_send(connection);

                const auto records = BoltConnection_fetch_summary(connection, run1);
                REQUIRE(records==0);
                REQUIRE(BoltConnection_summary_success(connection)==0);
                REQUIRE(connection->status==BOLT_FAILED);
                REQUIRE(connection->error==BOLT_SERVER_FAILURE);

                auto failure_data = BoltConnection_failure(connection);
                REQUIRE(failure_data!=nullptr);

                struct BoltValue* code = BoltDictionary_value_by_key(failure_data, "code", strlen("code"));
                struct BoltValue* message = BoltDictionary_value_by_key(failure_data, "message", strlen("message"));
                REQUIRE(code!=nullptr);
                REQUIRE(BoltValue_type(code)==BOLT_STRING);
                REQUIRE(BoltString_equals(code, "Neo.ClientError.Statement.SyntaxError"));
                REQUIRE(message!=nullptr);
                REQUIRE(BoltValue_type(message)==BOLT_STRING);
            }

            THEN("reset should clear failure state") {
                const auto records = BoltConnection_fetch_summary(connection, run);
                REQUIRE(records==0);
                REQUIRE(BoltConnection_summary_success(connection)==0);
                REQUIRE(connection->status==BOLT_FAILED);
                REQUIRE(connection->error==BOLT_SERVER_FAILURE);

                auto status = BoltConnection_load_reset_request(connection);

                REQUIRE(status==0);
                REQUIRE(BoltConnection_failure(connection)==nullptr);

                const auto ack_failure = BoltConnection_last_request(connection);
                BoltConnection_send(connection);
                const auto records_1 = BoltConnection_fetch_summary(connection, ack_failure);
                REQUIRE(records_1==0);
                REQUIRE(BoltConnection_summary_success(connection)==1);
                REQUIRE(connection->status==BOLT_READY);
                REQUIRE(connection->error==BOLT_SUCCESS);
            }
        }
        BoltConnection_close(connection);
    }
}