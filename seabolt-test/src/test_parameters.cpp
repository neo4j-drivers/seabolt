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

#include <cmath>
#include "integration.hpp"
#include "catch.hpp"

#define REQUIRE_BOLT_NULL(value) { REQUIRE(BoltValue_type(value) == BOLT_NULL); }
#define REQUIRE_BOLT_BIT(value, x) { REQUIRE(BoltValue_type(value) == BOLT_BIT); REQUIRE(BoltBit_get(value) == (x)); }
#define REQUIRE_BOLT_INT64(value, x) { REQUIRE(BoltValue_type(value) == BOLT_INT64); REQUIRE(BoltInt64_get(value) == (x)); }
#define REQUIRE_BOLT_FLOAT64(value, x) { REQUIRE(BoltValue_type(value) == BOLT_FLOAT64); REQUIRE( BoltFloat64_get(value) == (x)); }
#define REQUIRE_BOLT_STRING(value, x, size_) { REQUIRE(BoltValue_type(value) == BOLT_STRING); REQUIRE(strncmp(BoltString_get(value), x, size_) == 0); REQUIRE((value)->size == (size_)); }
#define REQUIRE_BOLT_BYTE_ARRAY(value, size_) { REQUIRE(BoltValue_type(value) == BOLT_BYTE_ARRAY); REQUIRE((value)->size == (size_)); }
#define REQUIRE_BOLT_LIST(value, size_) { REQUIRE(BoltValue_type(value) == BOLT_LIST); REQUIRE((value)->size == (size_)); }
#define REQUIRE_BOLT_DICTIONARY(value, size_) { REQUIRE(BoltValue_type(value) == BOLT_DICTIONARY); REQUIRE((value)->size == (size_)); }
#define REQUIRE_BOLT_STRUCTURE(value, code, size_) { REQUIRE(BoltValue_type(value) == BOLT_STRUCTURE); REQUIRE(BoltStructure_code(value) == (code)); REQUIRE((value)->size == (size_)); }
#define REQUIRE_BOLT_SUCCESS(value) { REQUIRE(BoltValue_type(value) == BOLT_MESSAGE); REQUIRE(BoltMessage_code(value) == 0x70); }

#define PREPARE_RETURN_X(connection, x)\
    BoltConnection_set_cypher_template(connection, "RETURN $x", 9);\
    BoltConnection_set_n_cypher_parameters(connection, 1);\
    BoltConnection_set_cypher_parameter_key(connection, 0, "x", 1);\
    BoltValue * (x) = BoltConnection_cypher_parameter_value(connection, 0);

#define RUN_PULL_SEND(connection, result)\
    BoltConnection_load_run_request(connection);\
    BoltConnection_load_pull_request(connection, -1);\
    int (result) = BoltConnection_send_b(connection);


SCENARIO("Test null parameter", "[integration][ipv6][secure]")
{
    if (IS_ONLINE)
    {
        GIVEN("an open and initialised connection")
        {
            struct BoltConnection * connection = NEW_BOLT_CONNECTION();
            WHEN("successfully executed Cypher")
            {
                PREPARE_RETURN_X(connection, x);
                BoltValue_to_Null(x);
                RUN_PULL_SEND(connection, result);
                struct BoltValue * data = BoltConnection_data(connection);
                while (BoltConnection_fetch_b(connection, result))
                {
                    REQUIRE_BOLT_LIST(data, 1);
                    REQUIRE_BOLT_NULL(BoltList_value(data, 0));
                }
                REQUIRE_BOLT_SUCCESS(data);
            }
            BoltConnection_close_b(connection);
        }
    }
}

SCENARIO("Test bit in, bit out", "[integration][ipv6][secure]")
{
    if (IS_ONLINE)
    {
        GIVEN("an open and initialised connection")
        {
            struct BoltConnection * connection = NEW_BOLT_CONNECTION();
            WHEN("successfully executed Cypher")
            {
                PREPARE_RETURN_X(connection, x);
                BoltValue_to_Bit(x, 1);
                RUN_PULL_SEND(connection, result);
                struct BoltValue * data = BoltConnection_data(connection);
                while (BoltConnection_fetch_b(connection, result))
                {
                    REQUIRE_BOLT_LIST(data, 1);
                    REQUIRE_BOLT_BIT(BoltList_value(data, 0), 1);
                }
                REQUIRE_BOLT_SUCCESS(data);
            }
            BoltConnection_close_b(connection);
        }
    }
}

SCENARIO("Test bit array in, list of bits out", "[integration][ipv6][secure]")
{
    if (IS_ONLINE)
    {
        GIVEN("an open and initialised connection")
        {
            struct BoltConnection * connection = NEW_BOLT_CONNECTION();
            WHEN("successfully executed Cypher")
            {
                PREPARE_RETURN_X(connection, x);
                char array[2] = {0, 1};
                BoltValue_to_BitArray(x, &array[0], sizeof(array));
                RUN_PULL_SEND(connection, result);
                struct BoltValue * data = BoltConnection_data(connection);
                while (BoltConnection_fetch_b(connection, result))
                {
                    REQUIRE_BOLT_LIST(data, 1);
                    BoltValue * value = BoltList_value(data, 0);
                    REQUIRE_BOLT_LIST(value, 2);
                    REQUIRE_BOLT_BIT(BoltList_value(value, 0), 0);
                    REQUIRE_BOLT_BIT(BoltList_value(value, 1), 1);
                }
                REQUIRE_BOLT_SUCCESS(data);
            }
            BoltConnection_close_b(connection);
        }
    }
}

SCENARIO("Test byte in, integer out", "[integration][ipv6][secure]")
{
    if (IS_ONLINE)
    {
        GIVEN("an open and initialised connection")
        {
            struct BoltConnection * connection = NEW_BOLT_CONNECTION();
            WHEN("successfully executed Cypher")
            {
                PREPARE_RETURN_X(connection, x);
                BoltValue_to_Byte(x, 123);
                RUN_PULL_SEND(connection, result);
                struct BoltValue * data = BoltConnection_data(connection);
                while (BoltConnection_fetch_b(connection, result))
                {
                    REQUIRE_BOLT_LIST(data, 1);
                    REQUIRE_BOLT_INT64(BoltList_value(data, 0), 123);
                }
                REQUIRE_BOLT_SUCCESS(data);
            }
            BoltConnection_close_b(connection);
        }
    }
}

SCENARIO("Test byte array in, byte array out", "[integration][ipv6][secure]")
{
    if (IS_ONLINE)
    {
        GIVEN("an open and initialised connection")
        {
            struct BoltConnection * connection = NEW_BOLT_CONNECTION();
            WHEN("successfully executed Cypher")
            {
                PREPARE_RETURN_X(connection, x);
                char array[5] = {33, 44, 55, 66, 77};
                BoltValue_to_ByteArray(x, &array[0], sizeof(array));
                RUN_PULL_SEND(connection, result);
                struct BoltValue * data = BoltConnection_data(connection);
                while (BoltConnection_fetch_b(connection, result))
                {
                    REQUIRE_BOLT_LIST(data, 1);
                    REQUIRE_BOLT_BYTE_ARRAY(BoltList_value(data, 0), 5);
                }
                REQUIRE_BOLT_SUCCESS(data);
            }
            BoltConnection_close_b(connection);
        }
    }
}

SCENARIO("Test one-byte char in, string out", "[integration][ipv6][secure]")
{
    if (IS_ONLINE)
    {
        GIVEN("an open and initialised connection")
        {
            struct BoltConnection * connection = NEW_BOLT_CONNECTION();
            WHEN("successfully executed Cypher")
            {
                PREPARE_RETURN_X(connection, x);
                BoltValue_to_Char(x, 'A');
                RUN_PULL_SEND(connection, result);
                struct BoltValue * data = BoltConnection_data(connection);
                while (BoltConnection_fetch_b(connection, result))
                {
                    REQUIRE_BOLT_LIST(data, 1);
                    REQUIRE_BOLT_STRING(BoltList_value(data, 0), "A", 1);
                }
                REQUIRE_BOLT_SUCCESS(data);
            }
            BoltConnection_close_b(connection);
        }
    }
}

SCENARIO("Test two-byte char in, string out", "[integration][ipv6][secure]")
{
    if (IS_ONLINE)
    {
        GIVEN("an open and initialised connection")
        {
            struct BoltConnection * connection = NEW_BOLT_CONNECTION();
            WHEN("successfully executed Cypher")
            {
                PREPARE_RETURN_X(connection, x);
                BoltValue_to_Char(x, 0xC4 /* 'Ä' */);
                RUN_PULL_SEND(connection, result);
                struct BoltValue * data = BoltConnection_data(connection);
                while (BoltConnection_fetch_b(connection, result))
                {
                    REQUIRE_BOLT_LIST(data, 1);
                    REQUIRE_BOLT_STRING(BoltList_value(data, 0), "\xC3\x84", 2);
                }
                REQUIRE_BOLT_SUCCESS(data);
            }
            BoltConnection_close_b(connection);
        }
    }
}

SCENARIO("Test three-byte char in, string out", "[integration][ipv6][secure]")
{
    if (IS_ONLINE)
    {
        GIVEN("an open and initialised connection")
        {
            struct BoltConnection * connection = NEW_BOLT_CONNECTION();
            WHEN("successfully executed Cypher")
            {
                PREPARE_RETURN_X(connection, x);
                BoltValue_to_Char(x, 0x1E00 /* 'Ḁ' - Latin Capital Letter A with ring below */);
                RUN_PULL_SEND(connection, result);
                struct BoltValue * data = BoltConnection_data(connection);
                while (BoltConnection_fetch_b(connection, result))
                {
                    REQUIRE_BOLT_LIST(data, 1);
                    REQUIRE_BOLT_STRING(BoltList_value(data, 0), "\xE1\xB8\x80", 3);
                }
                REQUIRE_BOLT_SUCCESS(data);
            }
            BoltConnection_close_b(connection);
        }
    }
}

SCENARIO("Test four-byte char in, string out", "[integration][ipv6][secure]")
{
    if (IS_ONLINE)
    {
        GIVEN("an open and initialised connection")
        {
            struct BoltConnection * connection = NEW_BOLT_CONNECTION();
            WHEN("successfully executed Cypher")
            {
                PREPARE_RETURN_X(connection, x);
                BoltValue_to_Char(x, 0x1D400 /* MATHEMATICAL BOLD CAPITAL A */);
                RUN_PULL_SEND(connection, result);
                struct BoltValue * data = BoltConnection_data(connection);
                while (BoltConnection_fetch_b(connection, result))
                {
                    REQUIRE_BOLT_LIST(data, 1);
                    REQUIRE_BOLT_STRING(BoltList_value(data, 0), "\xF0\x9D\x90\x80", 4);
                }
                REQUIRE_BOLT_SUCCESS(data);
            }
            BoltConnection_close_b(connection);
        }
    }
}

SCENARIO("Test char array in, list of strings out", "[integration][ipv6][secure]")
{
    if (IS_ONLINE)
    {
        GIVEN("an open and initialised connection")
        {
            struct BoltConnection * connection = NEW_BOLT_CONNECTION();
            WHEN("successfully executed Cypher")
            {
                PREPARE_RETURN_X(connection, x);
                uint32_t array[6] = {0x07, 'A', '\'', 0xC4, 0x1E00, 0x1D400};
                BoltValue_to_CharArray(x, array, 6);
                RUN_PULL_SEND(connection, result);
                struct BoltValue * data = BoltConnection_data(connection);
                while (BoltConnection_fetch_b(connection, result))
                {
                    REQUIRE_BOLT_LIST(data, 1);
                    struct BoltValue * list = BoltList_value(data, 0);
                    REQUIRE_BOLT_LIST(list, 6);
                    REQUIRE_BOLT_STRING(BoltList_value(list, 0), "\x07", 1);
                    REQUIRE_BOLT_STRING(BoltList_value(list, 1), "A", 1);
                    REQUIRE_BOLT_STRING(BoltList_value(list, 2), "'", 1);
                    REQUIRE_BOLT_STRING(BoltList_value(list, 3), "\xC3\x84", 2);
                    REQUIRE_BOLT_STRING(BoltList_value(list, 4), "\xE1\xB8\x80",3);
                    REQUIRE_BOLT_STRING(BoltList_value(list, 5), "\xF0\x9D\x90\x80", 4);
                }
                REQUIRE_BOLT_SUCCESS(data);
            }
            BoltConnection_close_b(connection);
        }
    }
}

SCENARIO("Test dictionary in, dictionary out", "[integration][ipv6][secure]")
{
    if (IS_ONLINE)
    {
        GIVEN("an open and initialised connection")
        {
            struct BoltConnection * connection = NEW_BOLT_CONNECTION();
            WHEN("successfully executed Cypher")
            {
                PREPARE_RETURN_X(connection, x);
                BoltValue_to_Dictionary(x, 2);
                BoltDictionary_set_key(x, 0, "name", 4);
                BoltValue_to_String(BoltDictionary_value(x, 0), "Alice", 5);
                BoltDictionary_set_key(x, 1, "age", 3);
                BoltValue_to_Int8(BoltDictionary_value(x, 1), 33);
                RUN_PULL_SEND(connection, result);
                struct BoltValue * data = BoltConnection_data(connection);
                while (BoltConnection_fetch_b(connection, result))
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
                            REQUIRE_BOLT_INT64(BoltDictionary_value(dict, i), 33);
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
            BoltConnection_close_b(connection);
        }
    }
}

SCENARIO("Test int8 in, int64 out", "[integration][ipv6][secure]")
{
    if (IS_ONLINE)
    {
        GIVEN("an open and initialised connection")
        {
            struct BoltConnection * connection = NEW_BOLT_CONNECTION();
            WHEN("successfully executed Cypher")
            {
                PREPARE_RETURN_X(connection, x);
                BoltValue_to_Int8(x, 123);
                RUN_PULL_SEND(connection, result);
                struct BoltValue * data = BoltConnection_data(connection);
                while (BoltConnection_fetch_b(connection, result))
                {
                    REQUIRE_BOLT_LIST(data, 1);
                    struct BoltValue * list = BoltList_value(data, 0);
                    REQUIRE_BOLT_INT64(list, 123);
                }
                REQUIRE_BOLT_SUCCESS(data);
            }
            BoltConnection_close_b(connection);
        }
    }
}

SCENARIO("Test int16 in, int64 out", "[integration][ipv6][secure]")
{
    if (IS_ONLINE)
    {
        GIVEN("an open and initialised connection")
        {
            struct BoltConnection * connection = NEW_BOLT_CONNECTION();
            WHEN("successfully executed Cypher")
            {
                PREPARE_RETURN_X(connection, x);
                BoltValue_to_Int16(x, 12345);
                RUN_PULL_SEND(connection, result);
                struct BoltValue * data = BoltConnection_data(connection);
                while (BoltConnection_fetch_b(connection, result))
                {
                    REQUIRE_BOLT_LIST(data, 1);
                    struct BoltValue * list = BoltList_value(data, 0);
                    REQUIRE_BOLT_INT64(list, 12345);
                }
                REQUIRE_BOLT_SUCCESS(data);
            }
            BoltConnection_close_b(connection);
        }
    }
}

SCENARIO("Test int32 in, int64 out", "[integration][ipv6][secure]")
{
    if (IS_ONLINE)
    {
        GIVEN("an open and initialised connection")
        {
            struct BoltConnection * connection = NEW_BOLT_CONNECTION();
            WHEN("successfully executed Cypher")
            {
                PREPARE_RETURN_X(connection, x);
                BoltValue_to_Int32(x, 1234567);
                RUN_PULL_SEND(connection, result);
                struct BoltValue * data = BoltConnection_data(connection);
                while (BoltConnection_fetch_b(connection, result))
                {
                    REQUIRE_BOLT_LIST(data, 1);
                    struct BoltValue * list = BoltList_value(data, 0);
                    REQUIRE_BOLT_INT64(list, 1234567);
                }
                REQUIRE_BOLT_SUCCESS(data);
            }
            BoltConnection_close_b(connection);
        }
    }
}

SCENARIO("Test int64 in, int64 out", "[integration][ipv6][secure]")
{
    if (IS_ONLINE)
    {
        GIVEN("an open and initialised connection")
        {
            struct BoltConnection * connection = NEW_BOLT_CONNECTION();
            WHEN("successfully executed Cypher")
            {
                PREPARE_RETURN_X(connection, x);
                BoltValue_to_Int64(x, 123456789);
                RUN_PULL_SEND(connection, result);
                struct BoltValue * data = BoltConnection_data(connection);
                while (BoltConnection_fetch_b(connection, result))
                {
                    REQUIRE_BOLT_LIST(data, 1);
                    struct BoltValue * list = BoltList_value(data, 0);
                    REQUIRE_BOLT_INT64(list, 123456789);
                }
                REQUIRE_BOLT_SUCCESS(data);
            }
            BoltConnection_close_b(connection);
        }
    }
}

SCENARIO("Test int8 array in, list of int64 out", "[integration][ipv6][secure]")
{
    if (IS_ONLINE)
    {
        GIVEN("an open and initialised connection")
        {
            struct BoltConnection * connection = NEW_BOLT_CONNECTION();
            WHEN("successfully executed Cypher")
            {
                PREPARE_RETURN_X(connection, x);
                const size_t array_size = 23;
                int8_t array[array_size] = {-89, -55, -34, -21, -13, -8, -5, -3, -2, -1, -1, 0, 1, 1, 2, 3, 5, 8, 13,
                                            21, 34, 55, 89};
                BoltValue_to_Int8Array(x, array, array_size);
                RUN_PULL_SEND(connection, result);
                struct BoltValue * data = BoltConnection_data(connection);
                while (BoltConnection_fetch_b(connection, result))
                {
                    REQUIRE_BOLT_LIST(data, 1);
                    struct BoltValue * list = BoltList_value(data, 0);
                    REQUIRE_BOLT_LIST(list, array_size);
                }
                REQUIRE_BOLT_SUCCESS(data);
            }
            BoltConnection_close_b(connection);
        }
    }
}

SCENARIO("Test int16 array in, list of int64 out", "[integration][ipv6][secure]")
{
    if (IS_ONLINE)
    {
        GIVEN("an open and initialised connection")
        {
            struct BoltConnection * connection = NEW_BOLT_CONNECTION();
            WHEN("successfully executed Cypher")
            {
                PREPARE_RETURN_X(connection, x);
                const size_t array_size = 24;
                int16_t array[array_size] = {0, 1, 1, 2, 3, 5, 8, 13, 21, 34, 55, 89, 144, 233, 377, 610, 987, 1597,
                                             2584, 4181, 6765, 10946, 17711, 28657};
                BoltValue_to_Int16Array(x, array, array_size);
                RUN_PULL_SEND(connection, result);
                struct BoltValue * data = BoltConnection_data(connection);
                while (BoltConnection_fetch_b(connection, result))
                {
                    REQUIRE_BOLT_LIST(data, 1);
                    struct BoltValue * list = BoltList_value(data, 0);
                    REQUIRE_BOLT_LIST(list, array_size);
                }
                REQUIRE_BOLT_SUCCESS(data);
            }
            BoltConnection_close_b(connection);
        }
    }
}

SCENARIO("Test int32 array in, list of int64 out", "[integration][ipv6][secure]")
{
    if (IS_ONLINE)
    {
        GIVEN("an open and initialised connection")
        {
            struct BoltConnection * connection = NEW_BOLT_CONNECTION();
            WHEN("successfully executed Cypher")
            {
                PREPARE_RETURN_X(connection, x);
                const size_t array_size = 47;
                int32_t array[array_size] = {0, 1, 1, 2, 3, 5, 8, 13, 21, 34, 55, 89, 144, 233, 377, 610, 987, 1597,
                                             2584, 4181, 6765, 10946, 17711, 28657, 46368, 75025, 121393, 196418,
                                             317811, 514229, 832040, 1346269, 2178309, 3524578, 5702887, 9227465,
                                             14930352, 24157817, 39088169, 63245986, 102334155, 165580141, 267914296,
                                             433494437, 701408733, 1134903170, 1836311903};
                BoltValue_to_Int32Array(x, array, array_size);
                RUN_PULL_SEND(connection, result);
                struct BoltValue * data = BoltConnection_data(connection);
                while (BoltConnection_fetch_b(connection, result))
                {
                    REQUIRE_BOLT_LIST(data, 1);
                    struct BoltValue * list = BoltList_value(data, 0);
                    REQUIRE_BOLT_LIST(list, array_size);
                }
                REQUIRE_BOLT_SUCCESS(data);
            }
            BoltConnection_close_b(connection);
        }
    }
}

SCENARIO("Test int64 array in, list of int64 out", "[integration][ipv6][secure]")
{
    if (IS_ONLINE)
    {
        GIVEN("an open and initialised connection")
        {
            struct BoltConnection * connection = NEW_BOLT_CONNECTION();
            WHEN("successfully executed Cypher")
            {
                PREPARE_RETURN_X(connection, x);
                const size_t array_size = 47;
                int64_t array[array_size] = {0, 1, 1, 2, 3, 5, 8, 13, 21, 34, 55, 89, 144, 233, 377, 610, 987, 1597,
                                             2584, 4181, 6765, 10946, 17711, 28657, 46368, 75025, 121393, 196418,
                                             317811, 514229, 832040, 1346269, 2178309, 3524578, 5702887, 9227465,
                                             14930352, 24157817, 39088169, 63245986, 102334155, 165580141, 267914296,
                                             433494437, 701408733, 1134903170, 1836311903};
                BoltValue_to_Int64Array(x, array, array_size);
                RUN_PULL_SEND(connection, result);
                struct BoltValue * data = BoltConnection_data(connection);
                while (BoltConnection_fetch_b(connection, result))
                {
                    REQUIRE_BOLT_LIST(data, 1);
                    struct BoltValue * list = BoltList_value(data, 0);
                    REQUIRE_BOLT_LIST(list, array_size);
                }
                REQUIRE_BOLT_SUCCESS(data);
            }
            BoltConnection_close_b(connection);
        }
    }
}

SCENARIO("Test float64 in, float64 out", "[integration][ipv6][secure]")
{
    if (IS_ONLINE)
    {
        GIVEN("an open and initialised connection")
        {
            struct BoltConnection * connection = NEW_BOLT_CONNECTION();
            WHEN("successfully executed Cypher")
            {
                PREPARE_RETURN_X(connection, x);
                BoltValue_to_Float64(x, 6.283185307179);
                RUN_PULL_SEND(connection, result);
                struct BoltValue * data = BoltConnection_data(connection);
                while (BoltConnection_fetch_b(connection, result))
                {
                    REQUIRE_BOLT_LIST(data, 1);
                    struct BoltValue * n = BoltList_value(data, 0);
                    REQUIRE_BOLT_FLOAT64(n, 6.283185307179);
                }
                REQUIRE_BOLT_SUCCESS(data);
            }
            BoltConnection_close_b(connection);
        }
    }
}

SCENARIO("Test float64 tuple in, list of float64 out", "[integration][ipv6][secure]")
{
    if (IS_ONLINE)
    {
        GIVEN("an open and initialised connection")
        {
            struct BoltConnection * connection = NEW_BOLT_CONNECTION();
            WHEN("successfully executed Cypher")
            {
                PREPARE_RETURN_X(connection, x);
                double data_in[4] = {3.141592653589, 6.283185307179, -123.456789, NAN};
                BoltValue_to_Float64Tuple(x, &data_in[0], 4);
                RUN_PULL_SEND(connection, result);
                struct BoltValue * data = BoltConnection_data(connection);
                while (BoltConnection_fetch_b(connection, result))
                {
                    REQUIRE_BOLT_LIST(data, 1);
                    struct BoltValue * tuple = BoltList_value(data, 0);
                    REQUIRE_BOLT_LIST(tuple, 4);
                    REQUIRE_BOLT_FLOAT64(BoltList_value(tuple, 0), 3.141592653589);
                    REQUIRE_BOLT_FLOAT64(BoltList_value(tuple, 1), 6.283185307179);
                    REQUIRE_BOLT_FLOAT64(BoltList_value(tuple, 2), -123.456789);
                    REQUIRE(std::isnan(BoltFloat64_get(BoltList_value(tuple, 3))));
                }
                REQUIRE_BOLT_SUCCESS(data);
            }
            BoltConnection_close_b(connection);
        }
    }
}

SCENARIO("Test float64 array in, list of float64 out", "[integration][ipv6][secure]")
{
    if (IS_ONLINE)
    {
        GIVEN("an open and initialised connection")
        {
            struct BoltConnection * connection = NEW_BOLT_CONNECTION();
            WHEN("successfully executed Cypher")
            {
                PREPARE_RETURN_X(connection, x);
                const size_t array_size = 3;
                double array[array_size] = {1.23, 4.56, 7.89};
                BoltValue_to_Float64Array(x, array, array_size);
                RUN_PULL_SEND(connection, result);
                struct BoltValue * data = BoltConnection_data(connection);
                while (BoltConnection_fetch_b(connection, result))
                {
                    REQUIRE_BOLT_LIST(data, 1);
                    struct BoltValue * tuple = BoltList_value(data, 0);
                    REQUIRE_BOLT_LIST(tuple, 3);
                    REQUIRE_BOLT_FLOAT64(BoltList_value(tuple, 0), 1.23);
                    REQUIRE_BOLT_FLOAT64(BoltList_value(tuple, 1), 4.56);
                    REQUIRE_BOLT_FLOAT64(BoltList_value(tuple, 2), 7.89);
                }
                REQUIRE_BOLT_SUCCESS(data);
            }
            BoltConnection_close_b(connection);
        }
    }
}

SCENARIO("Test float64 tuple array in, list of lists of float64 out", "[integration][ipv6][secure]")
{
    if (IS_ONLINE)
    {
        GIVEN("an open and initialised connection")
        {
            struct BoltConnection * connection = NEW_BOLT_CONNECTION();
            WHEN("successfully executed Cypher")
            {
                PREPARE_RETURN_X(connection, x);
                const size_t array_size = 3;
                double array_in[4* array_size] = {
                        -1.23, 2.34, -3.45, 9.87,
                        3.45, -4.56, 5.67, 0.0,
                        -5.67, -6.78, 7.89, 3.141592653589,
                };
                BoltValue_to_Float64TupleArray(x, &array_in[0], 4, array_size);
                RUN_PULL_SEND(connection, result);
                struct BoltValue * data = BoltConnection_data(connection);
                while (BoltConnection_fetch_b(connection, result))
                {
                    REQUIRE_BOLT_LIST(data, 1);
                    struct BoltValue * array_out = BoltList_value(data, 0);
                    REQUIRE_BOLT_LIST(array_out, array_size);
                    for (int i = 0; i < array_size; i++)
                    {
                        struct BoltValue * tuple = BoltList_value(array_out, i);
                        REQUIRE_BOLT_LIST(tuple, 4);
                        REQUIRE_BOLT_FLOAT64(BoltList_value(tuple, 0), array_in[4 * i + 0]);
                        REQUIRE_BOLT_FLOAT64(BoltList_value(tuple, 1), array_in[4 * i + 1]);
                        REQUIRE_BOLT_FLOAT64(BoltList_value(tuple, 2), array_in[4 * i + 2]);
                        REQUIRE_BOLT_FLOAT64(BoltList_value(tuple, 3), array_in[4 * i + 3]);
                    }
                }
                REQUIRE_BOLT_SUCCESS(data);
            }
            BoltConnection_close_b(connection);
        }
    }
}


SCENARIO("Test structure in result", "[integration][ipv6][secure]")
{
    if (IS_ONLINE)
    {
        GIVEN("an open and initialised connection")
        {
            struct BoltConnection * connection = NEW_BOLT_CONNECTION();
            WHEN("successfully executed Cypher")
            {
                BoltConnection_load_begin_request(connection);
                const char * statement = "CREATE (a:Person {name:'Alice'}) RETURN a";
                BoltConnection_set_cypher_template(connection, statement, strlen(statement));
                BoltConnection_set_n_cypher_parameters(connection, 0);
                BoltConnection_load_run_request(connection);
                int result = BoltConnection_load_pull_request(connection, -1);
                BoltConnection_load_rollback_request(connection);
                int last = BoltConnection_send_b(connection);
                struct BoltValue * data = BoltConnection_data(connection);
                while (BoltConnection_fetch_b(connection, result))
                {
                    REQUIRE_BOLT_LIST(data, 1);
                    struct BoltValue * node = BoltList_value(data, 0);
                    REQUIRE_BOLT_STRUCTURE(node, 'N', 3);
                    BoltValue * id = BoltStructure_value(node, 0);
                    BoltValue * labels = BoltStructure_value(node, 1);
                    BoltValue * properties = BoltStructure_value(node, 2);
                    REQUIRE(BoltValue_type(id) == BOLT_INT64);
                    REQUIRE_BOLT_LIST(labels, 1);
                    REQUIRE_BOLT_STRING(BoltList_value(labels, 0), "Person", 6);
                    REQUIRE_BOLT_DICTIONARY(properties, 1);
                    REQUIRE(strcmp(BoltDictionary_get_key(properties, 0), "name") == 0);
                    REQUIRE_BOLT_STRING(BoltDictionary_value(properties, 0), "Alice", 5);
                }
                BoltConnection_fetch_summary_b(connection, last);
                REQUIRE_BOLT_SUCCESS(data);
            }
            BoltConnection_close_b(connection);
        }
    }
}
