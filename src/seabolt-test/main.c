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

#include <stdint.h>
#include <stdio.h>
#include <assert.h>

#include "../seabolt/values.h"
#include "dump.h"

void test_null()
{
    struct BoltValue value = bolt_value();
    bolt_put_null(&value);
    bolt_dump_ln(&value);
    assert(value.type == BOLT_NULL);
}

void test_bit()
{
    struct BoltValue value = bolt_value();
    for (char i = 0; i <= 1; i++)
    {
        bolt_put_bit(&value, i);
        bolt_dump_ln(&value);
        assert(value.type == BOLT_BIT);
        assert(bolt_get_bit(&value) == i);
    }
    bolt_put_null(&value);
}

void test_bit_array()
{
    struct BoltValue value = bolt_value();
    int32_t size = 2;
    char array[] = {0, 1};
    bolt_put_bit_array(&value, array, size);
    bolt_dump_ln(&value);
    assert(value.type == BOLT_BIT_ARRAY);
    for (int i = 0; i < size; i++)
    {
        assert(bolt_get_bit_array_at(&value, i) == array[i]);
    }
    bolt_put_null(&value);
}

void test_byte()
{
    struct BoltValue value = bolt_value();
    for (int i = CHAR_MIN; i <= CHAR_MAX; i++)
    {
        bolt_put_byte(&value, (char)(i));
        bolt_dump_ln(&value);
        assert(value.type == BOLT_BYTE);
        assert(bolt_get_byte(&value) == i);
    }
    bolt_put_null(&value);
}

void test_byte_array()
{
    struct BoltValue value = bolt_value();
    char array[] = ("\x00\x01\x02\x03\x04\x05\x06\x07\x08\t\n\x0b\x0c\r\x0e\x0f"
                    "\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1a\x1b\x1c\x1d\x1e\x1f"
                    " !\"#$%&'()*+,-./0123456789:;<=>?"
                    "@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_"
                    "`abcdefghijklmnopqrstuvwxyz{|}~\x7f"
                    "\x80\x81\x82\x83\x84\x85\x86\x87\x88\x89\x8a\x8b\x8c\x8d\x8e\x8f"
                    "\x90\x91\x92\x93\x94\x95\x96\x97\x98\x99\x9a\x9b\x9c\x9d\x9e\x9f"
                    "\xa0\xa1\xa2\xa3\xa4\xa5\xa6\xa7\xa8\xa9\xaa\xab\xac\xad\xae\xaf"
                    "\xb0\xb1\xb2\xb3\xb4\xb5\xb6\xb7\xb8\xb9\xba\xbb\xbc\xbd\xbe\xbf"
                    "\xc0\xc1\xc2\xc3\xc4\xc5\xc6\xc7\xc8\xc9\xca\xcb\xcc\xcd\xce\xcf"
                    "\xd0\xd1\xd2\xd3\xd4\xd5\xd6\xd7\xd8\xd9\xda\xdb\xdc\xdd\xde\xdf"
                    "\xe0\xe1\xe2\xe3\xe4\xe5\xe6\xe7\xe8\xe9\xea\xeb\xec\xed\xee\xef"
                    "\xf0\xf1\xf2\xf3\xf4\xf5\xf6\xf7\xf8\xf9\xfa\xfb\xfc\xfd\xfe\xff");
    int32_t size = sizeof(array) - 1;
    bolt_put_byte_array(&value, array, size);
    bolt_dump_ln(&value);
    assert(value.type == BOLT_BYTE_ARRAY);
    for (int i = 0; i < size; i++)
    {
        assert(bolt_get_byte_array_at(&value, i) == array[i]);
    }
    bolt_put_null(&value);
}

void _test_utf8(char* text, int32_t text_size)
{
    struct BoltValue value = bolt_value();
    bolt_put_utf8(&value, text, text_size);
    bolt_dump_ln(&value);
    assert(value.type == BOLT_UTF8);
    assert(value.data_bytes == text_size);
    char* stored_text = bolt_get_utf8(&value);
    assert(strncmp(text, stored_text, (size_t)(text_size)) == 0);
    bolt_put_null(&value);
}

void test_utf8()
{
    _test_utf8("", 0);
    _test_utf8("hello, world", 12);
    _test_utf8("there is a null character -> \x00 <- in the middle of this string", 62);
}

void test_utf8_array()
{
    struct BoltValue value = bolt_value();
    char* text;
    int32_t size;
    bolt_put_utf8_array(&value);
    bolt_put_utf8_array_next(&value, "hello", 5);
    bolt_put_utf8_array_next(&value, "world", 5);
    bolt_put_utf8_array_next(&value, "here is a very very very very very very very very long string", 61);
    bolt_put_utf8_array_next(&value, "", 0);
    bolt_put_utf8_array_next(&value, "that last one was empty!!", 25);
    bolt_dump_ln(&value);
    assert(value.type == BOLT_UTF8_ARRAY);
    assert(value.data_items == 5);

    text = bolt_get_utf8_array_at(&value, 0);
    size = bolt_get_utf8_array_size_at(&value, 0);
    assert(strncmp(text, "hello", (size_t)(size)) == 0);

    text = bolt_get_utf8_array_at(&value, 1);
    size = bolt_get_utf8_array_size_at(&value, 1);
    assert(strncmp(text, "world", (size_t)(size)) == 0);

    text = bolt_get_utf8_array_at(&value, 2);
    size = bolt_get_utf8_array_size_at(&value, 2);
    assert(strncmp(text, "here is a very very very very very very very very long string", (size_t)(size)) == 0);

    text = bolt_get_utf8_array_at(&value, 3);
    size = bolt_get_utf8_array_size_at(&value, 3);
    assert(strncmp(text, "", (size_t)(size)) == 0);

    text = bolt_get_utf8_array_at(&value, 4);
    size = bolt_get_utf8_array_size_at(&value, 4);
    assert(strncmp(text, "that last one was empty!!", (size_t)(size)) == 0);

    bolt_put_null(&value);
}

int test_num8()
{
    struct BoltValue value = bolt_value();
    int n = 0;
    unsigned long long x = 0, y = 1, z;
    while (x < 0x100)
    {
        bolt_put_num8(&value, (uint8_t)(x));
        bolt_dump_ln(&value);
        assert(value.type == BOLT_NUM8);
        assert(bolt_get_num8(&value) == x);
        n += 1, z = x + y, x = y, y = z;
    }
    bolt_put_null(&value);
    return n;
}

void test_num8_array(int size)
{
    struct BoltValue value = bolt_value();
    uint8_t array[size];
    int n = 0;
    unsigned long long x = 0, y = 1, z;
    while (x < 0x100)
    {
        array[n] = (uint8_t)(x);
        n += 1, z = x + y, x = y, y = z;
    }
    bolt_put_num8_array(&value, array, size);
    bolt_dump_ln(&value);
    assert(value.type == BOLT_NUM8_ARRAY);
    for (int i = 0; i < size; i++)
    {
        assert(bolt_get_num8_array_at(&value, i) == array[i]);
    }
    bolt_put_null(&value);
}

int test_num16()
{
    struct BoltValue value = bolt_value();
    int n = 0;
    unsigned long long x = 0, y = 1, z;
    while (x < 0x10000)
    {
        bolt_put_num16(&value, (uint16_t)(x));
        bolt_dump_ln(&value);
        assert(value.type == BOLT_NUM16);
        assert(bolt_get_num16(&value) == x);
        n += 1, z = x + y, x = y, y = z;
    }
    bolt_put_null(&value);
    return n;
}

void test_num16_array(int size)
{
    struct BoltValue value = bolt_value();
    uint16_t array[size];
    int n = 0;
    unsigned long long x = 0, y = 1, z;
    while (x < 0x10000)
    {
        array[n] = (uint16_t)(x);
        n += 1, z = x + y, x = y, y = z;
    }
    bolt_put_num16_array(&value, array, size);
    bolt_dump_ln(&value);
    assert(value.type == BOLT_NUM16_ARRAY);
    for (int i = 0; i < size; i++)
    {
        assert(bolt_get_num16_array_at(&value, i) == array[i]);
    }
    bolt_put_null(&value);
}

int test_num32()
{
    struct BoltValue value = bolt_value();
    int n = 0;
    unsigned long long x = 0, y = 1, z;
    while (x < 0x100000000)
    {
        bolt_put_num32(&value, (uint32_t)(x));
        bolt_dump_ln(&value);
        assert(value.type == BOLT_NUM32);
        assert(bolt_get_num32(&value) == x);
        n += 1, z = x + y, x = y, y = z;
    }
    bolt_put_null(&value);
    return n;
}

void test_num32_array(int size)
{
    struct BoltValue value = bolt_value();
    uint32_t array[size];
    int n = 0;
    unsigned long long x = 0, y = 1, z;
    while (x < 0x100000000)
    {
        array[n] = (uint32_t)(x);
        n += 1, z = x + y, x = y, y = z;
    }
    bolt_put_num32_array(&value, array, size);
    bolt_dump_ln(&value);
    assert(value.type == BOLT_NUM32_ARRAY);
    for (int i = 0; i < size; i++)
    {
        assert(bolt_get_num32_array_at(&value, i) == array[i]);
    }
    bolt_put_null(&value);
}

int test_num64()
{
    struct BoltValue value = bolt_value();
    int n = 0;
    unsigned long long x = 0, y = 1, z;
    while (x <= 0xFFFF000000000000L)
    {
        bolt_put_num64(&value, (uint64_t)(x));
        bolt_dump_ln(&value);
        assert(value.type == BOLT_NUM64);
        assert(bolt_get_num64(&value) == x);
        n += 1, z = x + y, x = y, y = z;
    }
    bolt_put_null(&value);
    return n;
}

void test_num64_array(int size)
{
    struct BoltValue value = bolt_value();
    uint64_t array[size];
    int n = 0;
    unsigned long long x = 0, y = 1, z;
    while (x <= 0xFFFF000000000000L)
    {
        array[n] = (uint64_t)(x);
        n += 1, z = x + y, x = y, y = z;
    }
    bolt_put_num64_array(&value, array, size);
    bolt_dump_ln(&value);
    assert(value.type == BOLT_NUM64_ARRAY);
    for (int i = 0; i < size; i++)
    {
        assert(bolt_get_num64_array_at(&value, i) == array[i]);
    }
    bolt_put_null(&value);
}

int test_int8()
{
    struct BoltValue value = bolt_value();
    int n = 0, s = 1;
    unsigned long long x = 0, y = 1, z;
    while (x < 0x80)
    {
        bolt_put_int8(&value, (int8_t)(s * x));
        bolt_dump_ln(&value);
        assert(value.type == BOLT_INT8);
        assert(bolt_get_int8(&value) == s * x);
        n += 1, s = -s, z = x + y, x = y, y = z;
    }
    bolt_put_null(&value);
    return n;
}

void test_int8_array(int size)
{
    struct BoltValue value = bolt_value();
    int8_t array[size];
    int n = 0, s = 1;
    unsigned long long x = 0, y = 1, z;
    while (x < 0x80)
    {
        array[n] = (int8_t)(s * x);
        n += 1, s = -s, z = x + y, x = y, y = z;
    }
    bolt_put_int8_array(&value, array, size);
    bolt_dump_ln(&value);
    assert(value.type == BOLT_INT8_ARRAY);
    for (int i = 0; i < size; i++)
    {
        assert(bolt_get_int8_array_at(&value, i) == array[i]);
    }
    bolt_put_null(&value);
}

int test_int16()
{
    struct BoltValue value = bolt_value();
    int n = 0, s = 1;
    unsigned long long x = 0, y = 1, z;
    while (x < 0x8000)
    {
        bolt_put_int16(&value, (int16_t)(s * x));
        bolt_dump_ln(&value);
        assert(value.type == BOLT_INT16);
        assert(bolt_get_int16(&value) == s * x);
        n += 1, s = -s, z = x + y, x = y, y = z;
    }
    bolt_put_null(&value);
    return n;
}

void test_int16_array(int size)
{
    struct BoltValue value = bolt_value();
    int16_t array[size];
    int n = 0, s = 1;
    unsigned long long x = 0, y = 1, z;
    while (x < 0x8000)
    {
        array[n] = (int16_t)(s * x);
        n += 1, s = -s, z = x + y, x = y, y = z;
    }
    bolt_put_int16_array(&value, array, size);
    bolt_dump_ln(&value);
    assert(value.type == BOLT_INT16_ARRAY);
    for (int i = 0; i < size; i++)
    {
        assert(bolt_get_int16_array_at(&value, i) == array[i]);
    }
    bolt_put_null(&value);
}

int test_int32()
{
    struct BoltValue value = bolt_value();
    int n = 0, s = 1;
    unsigned long long x = 0, y = 1, z;
    while (x < 0x80000000)
    {
        bolt_put_int32(&value, (int32_t)(s * x));
        bolt_dump_ln(&value);
        assert(value.type == BOLT_INT32);
        assert(bolt_get_int32(&value) == s * x);
        n += 1, s = -s, z = x + y, x = y, y = z;
    }
    bolt_put_null(&value);
    return n;
}

void test_int32_array(int size)
{
    struct BoltValue value = bolt_value();
    int32_t array[size];
    int n = 0, s = 1;
    unsigned long long x = 0, y = 1, z;
    while (x < 0x80000000)
    {
        array[n] = (int32_t)(s * x);
        n += 1, s = -s, z = x + y, x = y, y = z;
    }
    bolt_put_int32_array(&value, array, size);
    bolt_dump_ln(&value);
    assert(value.type == BOLT_INT32_ARRAY);
    for (int i = 0; i < size; i++)
    {
        assert(bolt_get_int32_array_at(&value, i) == array[i]);
    }
    bolt_put_null(&value);
}

int test_int64()
{
    struct BoltValue value = bolt_value();
    int n = 0, s = 1;
    unsigned long long x = 0, y = 1, z;
    while (x < 0x8000000000000000)
    {
        bolt_put_int64(&value, (int64_t)(s * x));
        bolt_dump_ln(&value);
        assert(value.type == BOLT_INT64);
        assert(bolt_get_int64(&value) == s * x);
        n += 1, s = -s, z = x + y, x = y, y = z;
    }
    bolt_put_null(&value);
    return n;
}

void test_int64_array(int size)
{
    struct BoltValue value = bolt_value();
    int64_t array[size];
    int n = 0, s = 1;
    unsigned long long x = 0, y = 1, z;
    while (x < 0x8000000000000000)
    {
        array[n] = (int64_t)(s * x);
        n += 1, s = -s, z = x + y, x = y, y = z;
    }
    bolt_put_int64_array(&value, array, size);
    bolt_dump_ln(&value);
    assert(value.type == BOLT_INT64_ARRAY);
    for (int i = 0; i < size; i++)
    {
        assert(bolt_get_int64_array_at(&value, i) == array[i]);
    }
    bolt_put_null(&value);
}

int main()
{
    test_null();
    test_bit();
    test_bit_array();
    test_byte();
    test_byte_array();
    test_utf8();
    test_utf8_array();
    test_num8_array(test_num8());
    test_num16_array(test_num16());
    test_num32_array(test_num32());
    test_num64_array(test_num64());
    test_int8_array(test_int8());
    test_int16_array(test_int16());
    test_int32_array(test_int32());
    test_int64_array(test_int64());

//    const int NODE = 0xA0;
//    bolt_put_structure(&value, NODE, 3);
//    bolt_put_int64(bolt_get_structure_at(&value, 0), 123);
//    bolt_put_utf8_array(bolt_get_structure_at(&value, 1));
//    bolt_put_utf8_array_next(bolt_get_structure_at(&value, 1), "Person", 6);

}
