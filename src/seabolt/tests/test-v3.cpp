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

#include <string>
#include <utils/test-context.h>
#include "integration.hpp"
#include "catch.hpp"

using Catch::Matchers::Equals;

#define CONNECTION_ID_KEY "connection_id"
#define CONNECTION_ID_KEY_SIZE 13

TEST_CASE("Extract metadata", "[unit]")
{
    GIVEN("an open and initialised connection") {
        TestContext* test_ctx = new TestContext();
        struct BoltConnection* connection = bolt_open_init_mocked(3, test_ctx->log());

        WHEN("new connection_id would not overrun buffer") {
            BoltValue* metadata = BoltValue_create();
            BoltValue_format_as_Dictionary(metadata, 1);
            BoltDictionary_set_key(metadata, 0, CONNECTION_ID_KEY, CONNECTION_ID_KEY_SIZE);

            std::string old_connection_id = BoltConnection_id(connection);

            std::string value = "foo";
            BoltValue_format_as_String(
                    BoltDictionary_value(metadata, 0), value.c_str(), (int32_t) value.length());
            BoltProtocolV3_extract_metadata(connection, metadata);

            std::string connection_id = BoltConnection_id(connection);

            REQUIRE(old_connection_id.length()>0);
            THEN("it should not be concatenated to a blank with comma") {
                REQUIRE_THAT(connection_id, Equals(
                        old_connection_id+", "+value
                ));
            }

            BoltValue_destroy(metadata);
        }

        WHEN("new connection_id would overrun buffer") {
            BoltValue* metadata = BoltValue_create();
            BoltValue_format_as_Dictionary(metadata, 1);
            BoltDictionary_set_key(metadata, 0, CONNECTION_ID_KEY, CONNECTION_ID_KEY_SIZE);

            THEN("new connection_id should be ignored and the original connection_id should persist") {
                std::string old_connection_id = BoltConnection_id(connection);
                std::string value =
                        "0123456789""0123456789""0123456789""0123456789""0123456789"
                        "0123456789""0123456789""0123456789""0123456789""0123456789"
                        "0123456789""0123456789""0123456789""0123456789""0123456789"
                        "0123456789""0123456789""0123456789""0123456789""0123456789";
                BoltValue_format_as_String(BoltDictionary_value(metadata, 0), value.c_str(),
                        (int32_t) value.length());
                BoltProtocolV3_extract_metadata(connection, metadata);

                std::string connection_id = BoltConnection_id(connection);

                REQUIRE(connection_id.find(value)==std::string::npos);
                REQUIRE(connection_id==old_connection_id);
            }

            BoltValue_destroy(metadata);
        }

        BoltConnection_close(connection);
        BoltConnection_destroy(connection);
    }
}

TEST_CASE("Pass access mode", "[unit]")
{
    GIVEN("an open and initialised connection") {
        TestContext* test_ctx = new TestContext();
        struct BoltConnection* connection = bolt_open_init_mocked(3, test_ctx->log());

        BoltValue* tx_metadata = BoltValue_create();
        BoltValue_format_as_Dictionary(tx_metadata, 2);
        BoltDictionary_set_key(tx_metadata, 0, "m1", 2);
        BoltValue_format_as_Integer(BoltDictionary_value(tx_metadata, 0), 10);
        BoltDictionary_set_key(tx_metadata, 1, "m2", 2);
        BoltValue_format_as_Boolean(BoltDictionary_value(tx_metadata, 1), 1);

        BoltValue* bookmarks = BoltValue_create();
        BoltValue_format_as_List(bookmarks, 3);
        BoltValue_format_as_String(BoltList_value(bookmarks, 0), "bookmark-1", 10);
        BoltValue_format_as_String(BoltList_value(bookmarks, 1), "bookmark-2", 10);
        BoltValue_format_as_String(BoltList_value(bookmarks, 2), "bookmark-3", 10);

        WHEN("connection access mode is READ") {
            connection->access_mode = BOLT_ACCESS_MODE_READ;

            AND_WHEN("a BEGIN message is loaded") {
                BoltConnection_clear_begin(connection);
                BoltConnection_set_begin_tx_timeout(connection, 1000);
                BoltConnection_set_begin_tx_metadata(connection, tx_metadata);
                BoltConnection_set_begin_bookmarks(connection, bookmarks);
                BoltConnection_load_begin_request(connection);

                THEN("mode=r should be present in the statement metadata") {
                    REQUIRE_THAT(*test_ctx,
                            ContainsLog(
                                    "DEBUG: [id-0]: C[0] BEGIN [{tx_timeout: 1000, tx_metadata: {m1: 10, m2: true}, "
                                    "bookmarks: [bookmark-1, bookmark-2, bookmark-3], mode: r}]"));
                }
            }

            AND_WHEN("a RUN message is loaded") {
                BoltConnection_clear_run(connection);
                BoltConnection_set_run_cypher(connection, "RETURN $x", 9, 1);
                BoltValue_format_as_Integer(BoltConnection_set_run_cypher_parameter(connection, 0, "x", 1), 5);
                BoltConnection_set_run_tx_timeout(connection, 5000);
                BoltConnection_set_run_tx_metadata(connection, tx_metadata);
                BoltConnection_set_run_bookmarks(connection, bookmarks);
                BoltConnection_load_run_request(connection);

                THEN("mode=r should be present in the statement metadata") {
                    REQUIRE_THAT(*test_ctx, ContainsLog(
                            "DEBUG: [id-0]: C[0] RUN [RETURN $x, {x: 5}, {tx_timeout: 5000, tx_metadata: {m1: 10, m2: true}, "
                            "bookmarks: [bookmark-1, bookmark-2, bookmark-3], mode: r}]"));
                }
            }
        }

        WHEN("connection access mode is WRITE") {
            connection->access_mode = BOLT_ACCESS_MODE_WRITE;

            AND_WHEN("a BEGIN message is loaded") {
                BoltConnection_clear_begin(connection);
                BoltConnection_set_begin_tx_timeout(connection, 1000);
                BoltConnection_set_begin_tx_metadata(connection, tx_metadata);
                BoltConnection_set_begin_bookmarks(connection, bookmarks);
                BoltConnection_load_begin_request(connection);

                THEN("no access mode should be present in the statement metadata") {
                    REQUIRE_THAT(*test_ctx,
                            ContainsLog(
                                    "DEBUG: [id-0]: C[0] BEGIN [{tx_timeout: 1000, tx_metadata: {m1: 10, m2: true}, "
                                    "bookmarks: [bookmark-1, bookmark-2, bookmark-3]}]"));
                }
            }

            AND_WHEN("a RUN message is loaded") {
                BoltConnection_clear_run(connection);
                BoltConnection_set_run_cypher(connection, "RETURN $x", 9, 1);
                BoltValue_format_as_Integer(BoltConnection_set_run_cypher_parameter(connection, 0, "x", 1), 5);
                BoltConnection_set_run_tx_timeout(connection, 5000);
                BoltConnection_set_run_tx_metadata(connection, tx_metadata);
                BoltConnection_set_run_bookmarks(connection, bookmarks);
                BoltConnection_load_run_request(connection);

                THEN("no access mode should be present in the statement metadata") {
                    REQUIRE_THAT(*test_ctx,
                            ContainsLog(
                                    "DEBUG: [id-0]: C[0] RUN [RETURN $x, {x: 5}, {tx_timeout: 5000, tx_metadata: {m1: 10, m2: true}, "
                                    "bookmarks: [bookmark-1, bookmark-2, bookmark-3]}]"));
                }
            }
        }

        BoltValue_destroy(tx_metadata);
        BoltValue_destroy(bookmarks);
        BoltConnection_close(connection);
        BoltConnection_destroy(connection);
    }
}