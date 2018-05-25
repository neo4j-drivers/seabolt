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

#include <cmath>
#include "integration.hpp"
#include "catch.hpp"

#define REQUIRE_BOLT_NULL(value) { REQUIRE(BoltValue_type(value) == BOLT_NULL); }
#define REQUIRE_BOLT_BOOLEAN(value, x) { REQUIRE(BoltValue_type(value) == BOLT_BOOLEAN); REQUIRE(BoltBoolean_get(value) == (x)); }
#define REQUIRE_BOLT_INTEGER(value, x) { REQUIRE(BoltValue_type(value) == BOLT_INTEGER); REQUIRE(BoltInteger_get(value) == (x)); }
#define REQUIRE_BOLT_FLOAT(value, x) { REQUIRE(BoltValue_type(value) == BOLT_FLOAT); REQUIRE( BoltFloat_get(value) == (x)); }
#define REQUIRE_BOLT_STRING(value, x, size_) { REQUIRE(BoltValue_type(value) == BOLT_STRING); REQUIRE(strncmp(BoltString_get(value), x, size_) == 0); REQUIRE((value)->size == (size_)); }
#define REQUIRE_BOLT_DICTIONARY(value, size_) { REQUIRE(BoltValue_type(value) == BOLT_DICTIONARY); REQUIRE((value)->size == (size_)); }
#define REQUIRE_BOLT_LIST(value, size_) { REQUIRE(BoltValue_type(value) == BOLT_LIST); REQUIRE((value)->size == (size_)); }
#define REQUIRE_BOLT_BYTES(value, size_) { REQUIRE(BoltValue_type(value) == BOLT_BYTES); REQUIRE((value)->size == (size_)); }
#define REQUIRE_BOLT_STRUCTURE(value, code, size_) { REQUIRE(BoltValue_type(value) == BOLT_STRUCTURE); REQUIRE(BoltStructure_code(value) == (code)); REQUIRE((value)->size == (size_)); }
#define REQUIRE_BOLT_SUCCESS(value) { REQUIRE(BoltValue_type(value) == BOLT_MESSAGE); REQUIRE(BoltMessage_code(value) == 0x70); }

#define RUN_PULL_SEND(connection, result)\
    BoltConnection_load_run_request(connection);\
    BoltConnection_load_pull_request(connection, -1);\
    BoltConnection_send(connection);\
    bolt_request_t (result) = BoltConnection_last_request(connection);


SCENARIO("Test null parameter", "[integration][ipv6][secure]")
{
    GIVEN("an open and initialised connection")
    {
        struct BoltConnection * connection = NEW_BOLT_CONNECTION();
        WHEN("successfully executed Cypher")
        {
            const char * cypher = "RETURN $x";
            BoltConnection_cypher(connection, cypher, strlen(cypher), 1);
            BoltValue * x = BoltConnection_cypher_parameter(connection, 0, "x", 1);
            BoltValue_format_as_Null(x);
            RUN_PULL_SEND(connection, result);
            struct BoltValue * data = BoltConnection_data(connection);
            while (BoltConnection_fetch(connection, result))
            {
                REQUIRE_BOLT_LIST(data, 1);
                REQUIRE_BOLT_NULL(BoltList_value(data, 0));
            }
            REQUIRE_BOLT_SUCCESS(data);
        }
        bolt_close_and_destroy_b(connection);
    }
}

SCENARIO("Test boolean in, boolean out", "[integration][ipv6][secure]")
{
    GIVEN("an open and initialised connection")
    {
        struct BoltConnection * connection = NEW_BOLT_CONNECTION();
        WHEN("successfully executed Cypher")
        {
            const char * cypher = "RETURN $x";
            BoltConnection_cypher(connection, cypher, strlen(cypher), 1);
            BoltValue * x = BoltConnection_cypher_parameter(connection, 0, "x", 1);
            BoltValue_format_as_Boolean(x, 1);
            RUN_PULL_SEND(connection, result);
            struct BoltValue * data = BoltConnection_data(connection);
            while (BoltConnection_fetch(connection, result))
            {
                REQUIRE_BOLT_LIST(data, 1);
                REQUIRE_BOLT_BOOLEAN(BoltList_value(data, 0), 1);
            }
            REQUIRE_BOLT_SUCCESS(data);
        }
        bolt_close_and_destroy_b(connection);
    }
}

SCENARIO("Test bytes in, bytes out", "[integration][ipv6][secure]")
{
    GIVEN("an open and initialised connection")
    {
        struct BoltConnection * connection = NEW_BOLT_CONNECTION();
        WHEN("successfully executed Cypher")
        {
            const char * cypher = "RETURN $x";
            BoltConnection_cypher(connection, cypher, strlen(cypher), 1);
            BoltValue * x = BoltConnection_cypher_parameter(connection, 0, "x", 1);
            char array[5] = {33, 44, 55, 66, 77};
            BoltValue_format_as_Bytes(x, &array[0], sizeof(array));
            RUN_PULL_SEND(connection, result);
            struct BoltValue * data = BoltConnection_data(connection);
            while (BoltConnection_fetch(connection, result))
            {
                REQUIRE_BOLT_LIST(data, 1);
                REQUIRE_BOLT_BYTES(BoltList_value(data, 0), 5);
            }
            REQUIRE_BOLT_SUCCESS(data);
        }
        bolt_close_and_destroy_b(connection);
    }
}

SCENARIO("Test string in, string out", "[integration][ipv6][secure]")
{
    GIVEN("an open and initialised connection")
    {
        struct BoltConnection * connection = NEW_BOLT_CONNECTION();
        WHEN("successfully executed Cypher")
        {
            const char * cypher = "RETURN $x";
            BoltConnection_cypher(connection, cypher, strlen(cypher), 1);
            BoltValue * x = BoltConnection_cypher_parameter(connection, 0, "x", 1);
            BoltValue_format_as_String(x, "hello, world", 12);
            RUN_PULL_SEND(connection, result);
            struct BoltValue * data = BoltConnection_data(connection);
            while (BoltConnection_fetch(connection, result))
            {
                REQUIRE_BOLT_LIST(data, 1);
                REQUIRE_BOLT_STRING(BoltList_value(data, 0), "hello, world", 12);
            }
            REQUIRE_BOLT_SUCCESS(data);
        }
        bolt_close_and_destroy_b(connection);
    }
}

SCENARIO("Test dictionary in, dictionary out", "[integration][ipv6][secure]")
{
    GIVEN("an open and initialised connection")
    {
        struct BoltConnection * connection = NEW_BOLT_CONNECTION();
        WHEN("successfully executed Cypher")
        {
            const char * cypher = "RETURN $x";
            BoltConnection_cypher(connection, cypher, strlen(cypher), 1);
            BoltValue * x = BoltConnection_cypher_parameter(connection, 0, "x", 1);
            BoltValue_format_as_Dictionary(x, 2);
            BoltDictionary_set_key(x, 0, "name", 4);
            BoltValue_format_as_String(BoltDictionary_value(x, 0), "Alice", 5);
            BoltDictionary_set_key(x, 1, "age", 3);
            BoltValue_format_as_Integer(BoltDictionary_value(x, 1), 33);
            RUN_PULL_SEND(connection, result);
            struct BoltValue * data = BoltConnection_data(connection);
            while (BoltConnection_fetch(connection, result))
            {
                REQUIRE_BOLT_LIST(data, 1);
                struct BoltValue * dict = BoltList_value(data, 0);
                REQUIRE_BOLT_DICTIONARY(dict, 2);
                int found = 0;
                for (int i = 0; i < dict->size; i++)
                {
                    const char * key = BoltDictionary_get_key(dict, i);
                    if (strcmp(key, "name") == 0)
                    {
                        REQUIRE_BOLT_STRING(BoltDictionary_value(dict, i), "Alice", 5);
                        found += 1;
                    }
                    else if (strcmp(key, "age") == 0)
                    {
                        REQUIRE_BOLT_INTEGER(BoltDictionary_value(dict, i), 33);
                        found += 1;
                    }
                    else
                    {
                        FAIL();
                    }
                }
                REQUIRE(found == 2);
            }
            REQUIRE_BOLT_SUCCESS(data);
        }
        bolt_close_and_destroy_b(connection);
    }
}

SCENARIO("Test integer in, integer out", "[integration][ipv6][secure]")
{
    GIVEN("an open and initialised connection")
    {
        struct BoltConnection * connection = NEW_BOLT_CONNECTION();
        WHEN("successfully executed Cypher")
        {
            const char * cypher = "RETURN $x";
            BoltConnection_cypher(connection, cypher, strlen(cypher), 1);
            BoltValue * x = BoltConnection_cypher_parameter(connection, 0, "x", 1);
            BoltValue_format_as_Integer(x, 123456789);
            RUN_PULL_SEND(connection, result);
            struct BoltValue * data = BoltConnection_data(connection);
            while (BoltConnection_fetch(connection, result))
            {
                REQUIRE_BOLT_LIST(data, 1);
                struct BoltValue * list = BoltList_value(data, 0);
                REQUIRE_BOLT_INTEGER(list, 123456789);
            }
            REQUIRE_BOLT_SUCCESS(data);
        }
        bolt_close_and_destroy_b(connection);
    }
}

SCENARIO("Test float in, float out", "[integration][ipv6][secure]")
{
    GIVEN("an open and initialised connection")
    {
        struct BoltConnection * connection = NEW_BOLT_CONNECTION();
        WHEN("successfully executed Cypher")
        {
            const char * cypher = "RETURN $x";
            BoltConnection_cypher(connection, cypher, strlen(cypher), 1);
            BoltValue * x = BoltConnection_cypher_parameter(connection, 0, "x", 1);
            BoltValue_format_as_Float(x, 6.283185307179);
            RUN_PULL_SEND(connection, result);
            struct BoltValue * data = BoltConnection_data(connection);
            while (BoltConnection_fetch(connection, result))
            {
                REQUIRE_BOLT_LIST(data, 1);
                struct BoltValue * n = BoltList_value(data, 0);
                REQUIRE_BOLT_FLOAT(n, 6.283185307179);
            }
            REQUIRE_BOLT_SUCCESS(data);
        }
        bolt_close_and_destroy_b(connection);
    }
}

SCENARIO("Test structure in result", "[integration][ipv6][secure]")
{
    GIVEN("an open and initialised connection")
    {
        struct BoltConnection * connection = NEW_BOLT_CONNECTION();
        WHEN("successfully executed Cypher")
        {
            BoltConnection_load_begin_request(connection);
            const char * cypher = "CREATE (a:Person {name:'Alice'}) RETURN a";
            BoltConnection_cypher(connection, cypher, strlen(cypher), 0);
            BoltConnection_load_run_request(connection);
            BoltConnection_load_pull_request(connection, -1);
            bolt_request_t result = BoltConnection_last_request(connection);
            BoltConnection_load_rollback_request(connection);
            BoltConnection_send(connection);
            bolt_request_t last = BoltConnection_last_request(connection);
            struct BoltValue * data = BoltConnection_data(connection);
            while (BoltConnection_fetch(connection, result))
            {
                REQUIRE_BOLT_LIST(data, 1);
                struct BoltValue * node = BoltList_value(data, 0);
                REQUIRE_BOLT_STRUCTURE(node, 'N', 3);
                BoltValue * id = BoltStructure_value(node, 0);
                BoltValue * labels = BoltStructure_value(node, 1);
                BoltValue * properties = BoltStructure_value(node, 2);
                REQUIRE(BoltValue_type(id) == BOLT_INTEGER);
                REQUIRE_BOLT_LIST(labels, 1);
                REQUIRE_BOLT_STRING(BoltList_value(labels, 0), "Person", 6);
                REQUIRE_BOLT_DICTIONARY(properties, 1);
                REQUIRE(strcmp(BoltDictionary_get_key(properties, 0), "name") == 0);
                REQUIRE_BOLT_STRING(BoltDictionary_value(properties, 0), "Alice", 5);
            }
            BoltConnection_fetch_summary(connection, last);
            REQUIRE_BOLT_SUCCESS(data);
        }
        bolt_close_and_destroy_b(connection);
    }
}
