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

#include "integration.hpp"
#include "catch.hpp"


SCENARIO("Test address resolution (IPv4)", "[dns]")
{
    if (IS_ONLINE)
    {
        const char * host = "ipv4-only.bolt-test.net";
        const char * port = "7687";
        struct BoltAddress * address = BoltAddress_create(host, port);
        REQUIRE(strcmp(address->host, host) == 0);
        REQUIRE(strcmp(address->port, port) == 0);
        REQUIRE(address->n_resolved_hosts == 0);
        REQUIRE(address->resolved_port == 0);
        for (int i = 0; i < 2; i++)
        {
            BoltAddress_resolve_b(address);
            REQUIRE(address->n_resolved_hosts == 1);
            char host_string[40];
            int af = BoltAddress_copy_resolved_host(address, 0, &host_string[0], sizeof(host_string));
            REQUIRE(af == AF_INET);
			REQUIRE(strcmp(host_string, "52.215.65.80") == 0);
        	REQUIRE(address->resolved_port == 7687);
        }
        BoltAddress_destroy(address);
    }
}

SCENARIO("Test address resolution (IPv6)", "[dns]")
{
    if (IS_ONLINE)
    {
        const char * host = "ipv6-only.bolt-test.net";
        const char * port = "7687";
        struct BoltAddress * address = BoltAddress_create(host, port);
        REQUIRE(strcmp(address->host, host) == 0);
        REQUIRE(strcmp(address->port, port) == 0);
        REQUIRE(address->n_resolved_hosts == 0);
        REQUIRE(address->resolved_port == 0);
        for (int i = 0; i < 2; i++)
        {
            int status = BoltAddress_resolve_b(address);
            if (status == 0)
            {
                REQUIRE(address->n_resolved_hosts == 1);
                char host_string[40];
                int af = BoltAddress_copy_resolved_host(address, 0, &host_string[0], sizeof(host_string));
                REQUIRE(af == AF_INET6);
				REQUIRE(strcmp(host_string, "2a05:d018:1ca:6113:c9d8:4689:33f2:15f7") == 0);
                REQUIRE(address->resolved_port == 7687);
            }
        }
        BoltAddress_destroy(address);
    }
}

SCENARIO("Test address resolution (IPv4 and IPv6)", "[dns]")
{
    if (IS_ONLINE)
    {
        const char * host = "ipv4-and-ipv6.bolt-test.net";
        const char * port = "7687";
        struct BoltAddress * address = BoltAddress_create(host, port);
        REQUIRE(strcmp(address->host, host) == 0);
        REQUIRE(strcmp(address->port, port) == 0);
        REQUIRE(address->n_resolved_hosts == 0);
        REQUIRE(address->resolved_port == 0);
        for (int i = 0; i < 2; i++)
        {
            int status = BoltAddress_resolve_b(address);
            if (status == 0)
            {
                for (size_t j = 0; j < address->n_resolved_hosts; j++)
                {
                    char host_string[40];
					int af = BoltAddress_copy_resolved_host(address, j, &host_string[0], sizeof(host_string));
                    switch (af)
                    {
                        case AF_INET:
						    REQUIRE(strcmp(host_string, "52.215.65.80") == 0);
                            break;
                        case AF_INET6:
    						REQUIRE(strcmp(host_string, "2a05:d018:1ca:6113:c9d8:4689:33f2:15f7") == 0);
                            break;
                        default:
                            FAIL();
                    }
                }
                REQUIRE(address->resolved_port == 7687);
            }
        }
        BoltAddress_destroy(address);
    }
}

SCENARIO("Test basic secure connection (IPv4)", "[integration][ipv4][secure]")
{
    if (IS_ONLINE)
    {
        GIVEN("a local server address")
        {
            struct BoltAddress * address = bolt_get_address(BOLT_IPV4_HOST, BOLT_PORT);
            WHEN("a secure connection is opened")
            {
                struct BoltConnection * connection = BoltConnection_open_b(BOLT_SECURE_SOCKET, address);
                THEN("the connection should be connected")
                {
                    REQUIRE(connection->status == BOLT_CONNECTED);
                }
                BoltConnection_close_b(connection);
            }
            BoltAddress_destroy(address);
        }
    }
}

SCENARIO("Test basic secure connection (IPv6)", "[integration][ipv6][secure]")
{
    if (IS_ONLINE)
    {
        GIVEN("a local server address")
        {
            struct BoltAddress * address = bolt_get_address(BOLT_IPV6_HOST, BOLT_PORT);
            WHEN("a secure connection is opened")
            {
                struct BoltConnection * connection = BoltConnection_open_b(BOLT_SECURE_SOCKET, address);
                THEN("the connection should be connected")
                {
                    REQUIRE(connection->status == BOLT_CONNECTED);
                }
                BoltConnection_close_b(connection);
            }
            BoltAddress_destroy(address);
        }
    }
}

SCENARIO("Test basic insecure connection (IPv4)", "[integration][ipv4][insecure]")
{
    if (IS_ONLINE)
    {
        GIVEN("a local server address")
        {
            struct BoltAddress * address = bolt_get_address(BOLT_IPV4_HOST, BOLT_PORT);
            WHEN("an insecure connection is opened")
            {
                struct BoltConnection * connection = BoltConnection_open_b(BOLT_INSECURE_SOCKET, address);
                THEN("the connection should be connected")
                {
                    REQUIRE(connection->status == BOLT_CONNECTED);
                }
                BoltConnection_close_b(connection);
            }
            BoltAddress_destroy(address);
        }
    }
}

SCENARIO("Test basic insecure connection (IPv6)", "[integration][ipv6][insecure]")
{
    if (IS_ONLINE)
    {
        GIVEN("a local server address")
        {
            struct BoltAddress * address = bolt_get_address(BOLT_IPV6_HOST, BOLT_PORT);
            WHEN("an insecure connection is opened")
            {
                struct BoltConnection * connection = BoltConnection_open_b(BOLT_INSECURE_SOCKET, address);
                THEN("the connection should be connected")
                {
                    REQUIRE(connection->status == BOLT_CONNECTED);
                }
                BoltConnection_close_b(connection);
            }
            BoltAddress_destroy(address);
        }
    }
}

SCENARIO("Test secure connection to dead port", "[integration][ipv6][secure]")
{
    if (IS_ONLINE)
    {
        GIVEN("a local server address")
        {
            struct BoltAddress * address = bolt_get_address(BOLT_IPV6_HOST, "9999");
            WHEN("a secure connection attempt is made")
            {
                struct BoltConnection * connection = BoltConnection_open_b(BOLT_SECURE_SOCKET, address);
                THEN("a DEFUNCT connection should be returned")
                {
                    REQUIRE(connection->status == BOLT_DEFUNCT);
                }
                BoltConnection_close_b(connection);
            }
            BoltAddress_destroy(address);
        }
    }
}

SCENARIO("Test insecure connection to dead port", "[integration][ipv6][insecure]")
{
    if (IS_ONLINE)
    {
        GIVEN("a local server address")
        {
            struct BoltAddress * address = bolt_get_address(BOLT_IPV6_HOST, "9999");
            WHEN("an insecure connection attempt is made")
            {
                struct BoltConnection * connection = BoltConnection_open_b(BOLT_INSECURE_SOCKET, address);
                THEN("a DEFUNCT connection should be returned")
                {
                    REQUIRE(connection->status == BOLT_DEFUNCT);
                }
                BoltConnection_close_b(connection);
            }
            BoltAddress_destroy(address);
        }
    }
}

SCENARIO("Test init with valid credentials", "[integration][ipv6][secure]")
{
    if (IS_ONLINE)
    {
        GIVEN("an open connection")
        {
            struct BoltConnection * connection = bolt_open_b(BOLT_SECURE_SOCKET, BOLT_IPV6_HOST, BOLT_PORT);
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
}

SCENARIO("Test init with invalid credentials", "[integration][ipv6][secure]")
{
    if (IS_ONLINE)
    {
        GIVEN("an open connection")
        {
            struct BoltConnection * connection = bolt_open_b(BOLT_SECURE_SOCKET, BOLT_IPV6_HOST, BOLT_PORT);
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
}

SCENARIO("Test execution of simple Cypher statement", "[integration][ipv6][secure]")
{
    if (IS_ONLINE)
    {
        GIVEN("an open and initialised connection")
        {
            struct BoltConnection * connection = bolt_open_and_init_b(BOLT_SECURE_SOCKET, BOLT_IPV6_HOST, BOLT_PORT,
                                                                      BOLT_USER, BOLT_PASSWORD);
            WHEN("successfully executed Cypher")
            {
                BoltConnection_set_cypher_template(connection, "RETURN 1", 8);
                BoltConnection_set_n_cypher_parameters(connection, 0);
                int run = BoltConnection_load_run_request(connection);
                int pull = BoltConnection_load_pull_request(connection, -1);
                BoltConnection_send_b(connection);
                int records = BoltConnection_fetch_summary_b(connection, run);
                REQUIRE(records == 0);
                records = BoltConnection_fetch_summary_b(connection, pull);
                REQUIRE(records == 1);
            }
            BoltConnection_close_b(connection);
        }
    }
}

SCENARIO("Test field names returned from Cypher execution", "[integration][ipv6][secure][current]")
{
    if (IS_ONLINE)
    {
        GIVEN("an open and initialised connection")
        {
            struct BoltConnection * connection = bolt_open_and_init_b(BOLT_SECURE_SOCKET, BOLT_IPV6_HOST, BOLT_PORT,
                                                                      BOLT_USER, BOLT_PASSWORD);
            WHEN("successfully executed Cypher")
            {
                const char * statement = "RETURN 1 AS first, true AS second, 3.14 AS third";
                BoltConnection_set_cypher_template(connection, statement, strlen(statement));
                BoltConnection_set_n_cypher_parameters(connection, 0);
                int run = BoltConnection_load_run_request(connection);
                BoltConnection_load_pull_request(connection, -1);
                int last = BoltConnection_send_b(connection);
                BoltConnection_fetch_summary_b(connection, run);
                int n_fields = BoltConnection_n_fields(connection);
                REQUIRE(n_fields == 3);
                const char * field_name = BoltConnection_field_name(connection, 0);
                int field_name_size = BoltConnection_field_name_size(connection, 0);
                REQUIRE(strcmp(field_name, "first") == 0);
                REQUIRE(field_name_size == 5);
                field_name = BoltConnection_field_name(connection, 1);
                field_name_size = BoltConnection_field_name_size(connection, 1);
                REQUIRE(strcmp(field_name, "second") == 0);
                REQUIRE(field_name_size == 6);
                field_name = BoltConnection_field_name(connection, 2);
                field_name_size = BoltConnection_field_name_size(connection, 2);
                REQUIRE(strcmp(field_name, "third") == 0);
                REQUIRE(field_name_size == 5);
                BoltConnection_fetch_summary_b(connection, last);
            }
            BoltConnection_close_b(connection);
        }
    }
}

SCENARIO("Test parameterised Cypher statements", "[integration][ipv6][secure]")
{
    if (IS_ONLINE)
    {
        GIVEN("an open and initialised connection")
        {
            struct BoltConnection * connection = bolt_open_and_init_b(BOLT_SECURE_SOCKET, BOLT_IPV6_HOST, BOLT_PORT,
                                                                      BOLT_USER, BOLT_PASSWORD);
            WHEN("successfully executed Cypher")
            {
                BoltConnection_set_cypher_template(connection, "RETURN $x", 9);
                BoltConnection_set_n_cypher_parameters(connection, 1);
                BoltConnection_set_cypher_parameter_key(connection, 0, "x", 1);
                BoltValue * x = BoltConnection_cypher_parameter_value(connection, 0);
                BoltValue_to_Int64(x, 42);
                int run = BoltConnection_load_run_request(connection);
                int pull = BoltConnection_load_pull_request(connection, -1);
                BoltConnection_send_b(connection);
                int records = BoltConnection_fetch_summary_b(connection, run);
                REQUIRE(records == 0);
                struct BoltValue * last_received = BoltConnection_data(connection);
                REQUIRE(BoltValue_type(last_received) == BOLT_MESSAGE);
                REQUIRE(BoltMessage_code(last_received) == 0x70);
                while (BoltConnection_fetch_b(connection, pull))
                {
                    REQUIRE(BoltValue_type(last_received) == BOLT_LIST);
                    REQUIRE(last_received->size == 1);
                    BoltValue * value = BoltList_value(last_received, 0);
                    REQUIRE(BoltValue_type(value) == BOLT_INT64);
                    REQUIRE(BoltInt64_get(value) == 42);
                    records += 1;
                }
                REQUIRE(BoltValue_type(last_received) == BOLT_MESSAGE);
                REQUIRE(BoltMessage_code(last_received) == 0x70);
                REQUIRE(records == 1);
            }
            BoltConnection_close_b(connection);
        }
    }
}

SCENARIO("Test execution of multiple Cypher statements transmitted together", "[integration][ipv6][secure]")
{
    if (IS_ONLINE)
    {
        GIVEN("an open and initialised connection")
        {
            struct BoltConnection * connection = bolt_open_and_init_b(BOLT_SECURE_SOCKET, BOLT_IPV6_HOST, BOLT_PORT,
                                                                      BOLT_USER, BOLT_PASSWORD);
            WHEN("successfully executed Cypher")
            {
                BoltConnection_set_cypher_template(connection, "RETURN $x", 9);
                BoltConnection_set_n_cypher_parameters(connection, 1);
                BoltConnection_set_cypher_parameter_key(connection, 0, "x", 1);
                BoltValue * x = BoltConnection_cypher_parameter_value(connection, 0);
                BoltValue_to_Int8(x, 1);
                BoltConnection_load_run_request(connection);
                BoltConnection_load_discard_request(connection, -1);
                BoltValue_to_Int8(x, 2);
                BoltConnection_load_run_request(connection);
                BoltConnection_load_pull_request(connection, -1);
                int last = BoltConnection_send_b(connection);
                int records = BoltConnection_fetch_summary_b(connection, last);
                REQUIRE(records == 1);
            }
            BoltConnection_close_b(connection);
        }
    }
}

SCENARIO("Test transactions", "[integration][ipv6][secure]")
{
    if (IS_ONLINE)
    {
        GIVEN("an open and initialised connection")
        {
            struct BoltConnection * connection = bolt_open_and_init_b(BOLT_SECURE_SOCKET, BOLT_IPV6_HOST, BOLT_PORT,
                                                                      BOLT_USER, BOLT_PASSWORD);
            WHEN("successfully executed Cypher")
            {
                int begin = BoltConnection_load_begin_request(connection);

                BoltConnection_set_cypher_template(connection, "RETURN 1", 8);
                BoltConnection_set_n_cypher_parameters(connection, 0);
                int run = BoltConnection_load_run_request(connection);
                int pull = BoltConnection_load_pull_request(connection, -1);

                int commit = BoltConnection_load_commit_request(connection);

                int last = BoltConnection_send_b(connection);
                REQUIRE(last == commit);

                int records = BoltConnection_fetch_summary_b(connection, begin);
                REQUIRE(records == 0);
                struct BoltValue * fetched = BoltConnection_data(connection);
                REQUIRE(BoltValue_type(fetched) == BOLT_MESSAGE);
                REQUIRE(BoltMessage_code(fetched) == 0x70);

                records = BoltConnection_fetch_summary_b(connection, run);
                REQUIRE(records == 0);
                fetched = BoltConnection_data(connection);
                REQUIRE(BoltValue_type(fetched) == BOLT_MESSAGE);
                REQUIRE(BoltMessage_code(fetched) == 0x70);

                while (BoltConnection_fetch_b(connection, pull))
                {
                    REQUIRE(BoltValue_type(fetched) == BOLT_LIST);
                    REQUIRE(fetched->size == 1);
                    BoltValue * value = BoltList_value(fetched, 0);
                    REQUIRE(BoltValue_type(value) == BOLT_INT64);
                    REQUIRE(BoltInt64_get(value) == 1);
                    records += 1;
                }
                REQUIRE(BoltValue_type(fetched) == BOLT_MESSAGE);
                REQUIRE(BoltMessage_code(fetched) == 0x70);
                REQUIRE(records == 1);

                records = BoltConnection_fetch_summary_b(connection, commit);
                REQUIRE(records == 0);
                fetched = BoltConnection_data(connection);
                REQUIRE(BoltValue_type(fetched) == BOLT_MESSAGE);
                REQUIRE(BoltMessage_code(fetched) == 0x70);

            }
            BoltConnection_close_b(connection);
        }
    }
}
