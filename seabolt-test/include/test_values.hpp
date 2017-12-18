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

#ifndef SEABOLT_TEST_TYPES
#define SEABOLT_TEST_TYPES



void test_null();

void test_list();

void test_utf8_dictionary();

void test_bit();

void test_bit_array();

void test_byte();

void test_byte_array();

void test_utf8();

void test_utf8_array();

int test_num8();

void test_num8_array(int size);

int test_num16();

void test_num16_array(int size);

int test_num32();

void test_num32_array(int size);

int test_num64();

void test_num64_array(int size);

int test_int8();

void test_int8_array(int size);

int test_int16();

void test_int16_array(int size);

int test_int32();

void test_int32_array(int size);

int test_int64();

void test_int64_array(int size);

void test_float32();

void test_float32_array();

void test_float64();

void test_structure();

void test_structure_array();

void test_request();

void test_summary();

int test_types();


#endif // SEABOLT_TEST_TYPES
