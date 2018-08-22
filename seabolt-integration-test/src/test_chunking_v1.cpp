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

#include <cmath>
#include "integration.hpp"
#include "catch.hpp"

using Catch::Matchers::Equals;

#define REQUIRE_BOLT_NULL(value) { REQUIRE(BoltValue_type(value) == BOLT_NULL); }
#define REQUIRE_BOLT_BOOLEAN(value, x) { REQUIRE(BoltValue_type(value) == BOLT_BOOLEAN); REQUIRE(BoltBoolean_get(value) == (x)); }
#define REQUIRE_BOLT_INTEGER(value, x) { REQUIRE(BoltValue_type(value) == BOLT_INTEGER); REQUIRE(BoltInteger_get(value) == (x)); }
#define REQUIRE_BOLT_FLOAT(value, x) { REQUIRE(BoltValue_type(value) == BOLT_FLOAT); REQUIRE( BoltFloat_get(value) == (x)); }
#define REQUIRE_BOLT_STRING(value, x, size_) { REQUIRE(BoltValue_type(value) == BOLT_STRING); REQUIRE(strncmp(BoltString_get(value), x, size_) == 0); REQUIRE((value)->size == (int32_t)(size_)); }
#define REQUIRE_BOLT_DICTIONARY(value, size_) { REQUIRE(BoltValue_type(value) == BOLT_DICTIONARY); REQUIRE((value)->size == (int32_t)(size_)); }
#define REQUIRE_BOLT_LIST(value, size_) { REQUIRE(BoltValue_type(value) == BOLT_LIST); REQUIRE((value)->size == (int32_t)(size_)); }
#define REQUIRE_BOLT_BYTES(value, size_) { REQUIRE(BoltValue_type(value) == BOLT_BYTES); REQUIRE((value)->size == (int32_t)(size_)); }
#define REQUIRE_BOLT_STRUCTURE(value, code, size_) { REQUIRE(BoltValue_type(value) == BOLT_STRUCTURE); REQUIRE(BoltStructure_code(value) == (code)); REQUIRE((value)->size == (int32_t)(size_)); }
#define REQUIRE_BOLT_SUCCESS(connection) { REQUIRE(BoltConnection_summary_success(connection) == 1); }

#define RUN_PULL_SEND(connection, result)\
    BoltConnection_load_run_request(connection);\
    BoltConnection_load_pull_request(connection, -1);\
    BoltConnection_send(connection);\
    bolt_request_t (result) = BoltConnection_last_request(connection);

SCENARIO("Test chunking", "[integration][ipv6][secure]")
{
    GIVEN("an open and initialised connection") {
        struct BoltConnection* connection = bolt_open_init_default();
        WHEN("Cypher with parameter of small size") {
            const char* cypher = "RETURN $x";
            BoltConnection_cypher(connection, cypher, strlen(cypher), 1);
            BoltValue* x = BoltConnection_cypher_parameter(connection, 0, "x", 1);
            int param_size = 2;
            char* param = (char*) calloc(param_size, sizeof(char));
            for (int i = 1; i<param_size; i++) {
                param[i-1] = 'A'+(rand()%25);
            }
            BoltValue_format_as_String(x, param, (int32_t) strlen(param));
            RUN_PULL_SEND(connection, result);
            THEN("It should return passed parameter") {
                while (BoltConnection_fetch(connection, result)) {
                    const struct BoltValue* field_values = BoltConnection_record_fields(connection);
                    struct BoltValue* value = BoltList_value(field_values, 0);
                    REQUIRE_BOLT_STRING(value, param, strlen(param));
                }
                REQUIRE_BOLT_SUCCESS(connection);
            }
        }

        WHEN("Cypher with parameter of medium size") {
            const char* cypher = "RETURN $x";
            BoltConnection_cypher(connection, cypher, strlen(cypher), 1);
            BoltValue* x = BoltConnection_cypher_parameter(connection, 0, "x", 1);
            int param_size = 32769;
            char* param = (char*) calloc(param_size, sizeof(char));
            for (int i = 1; i<param_size; i++) {
                param[i-1] = 'A'+(rand()%25);
            }
            BoltValue_format_as_String(x, param, (int32_t) strlen(param));
            RUN_PULL_SEND(connection, result);
            THEN("It should return passed parameter") {
                while (BoltConnection_fetch(connection, result)) {
                    const struct BoltValue* field_values = BoltConnection_record_fields(connection);
                    struct BoltValue* value = BoltList_value(field_values, 0);
                    REQUIRE_BOLT_STRING(value, param, strlen(param));
                }
                REQUIRE_BOLT_SUCCESS(connection);
            }
        }

        WHEN("Cypher with parameter of boundary size") {
            const char* cypher = "RETURN $x";
            BoltConnection_cypher(connection, cypher, strlen(cypher), 1);
            BoltValue* x = BoltConnection_cypher_parameter(connection, 0, "x", 1);
            int param_size = 65536;
            char* param = (char*) calloc(param_size, sizeof(char));
            for (int i = 1; i<param_size; i++) {
                param[i-1] = 'A'+(rand()%25);
            }
            BoltValue_format_as_String(x, param, (int32_t) strlen(param));
            RUN_PULL_SEND(connection, result);
            THEN("It should return passed parameter") {
                while (BoltConnection_fetch(connection, result)) {
                    const struct BoltValue* field_values = BoltConnection_record_fields(connection);
                    struct BoltValue* value = BoltList_value(field_values, 0);
                    REQUIRE_BOLT_STRING(value, param, strlen(param));
                }
                REQUIRE_BOLT_SUCCESS(connection);
            }
        }

        WHEN("Cypher with parameter of large size") {
            const char* cypher = "RETURN $x";
            BoltConnection_cypher(connection, cypher, strlen(cypher), 1);
            BoltValue* x = BoltConnection_cypher_parameter(connection, 0, "x", 1);
            int param_size = 65535*2+1;
            char* param = (char*) calloc(param_size, sizeof(char));
            for (int i = 1; i<param_size; i++) {
                param[i-1] = 'A'+(rand()%25);
            }
            BoltValue_format_as_String(x, param, (int32_t) strlen(param));
            RUN_PULL_SEND(connection, result);
            THEN("It should return passed parameter") {
                while (BoltConnection_fetch(connection, result)) {
                    const struct BoltValue* field_values = BoltConnection_record_fields(connection);
                    struct BoltValue* value = BoltList_value(field_values, 0);
                    REQUIRE_BOLT_STRING(value, param, strlen(param));
                }
                REQUIRE_BOLT_SUCCESS(connection);
            }
        }

        WHEN("Cypher with parameter of very large size") {
            const char* cypher = "RETURN $x";
            BoltConnection_cypher(connection, cypher, strlen(cypher), 1);
            BoltValue* x = BoltConnection_cypher_parameter(connection, 0, "x", 1);
            int param_size = 65535*10+1;
            char* param = (char*) calloc(param_size, sizeof(char));
            for (int i = 1; i<param_size; i++) {
                param[i-1] = 'A'+(rand()%25);
            }
            BoltValue_format_as_String(x, param, (int32_t) strlen(param));
            RUN_PULL_SEND(connection, result);
            THEN("It should return passed parameter") {
                while (BoltConnection_fetch(connection, result)) {
                    const struct BoltValue* field_values = BoltConnection_record_fields(connection);
                    struct BoltValue* value = BoltList_value(field_values, 0);
                    REQUIRE_BOLT_STRING(value, param, strlen(param));
                }
                REQUIRE_BOLT_SUCCESS(connection);
            }
        }

        bolt_close_and_destroy_b(connection);
    }
}
