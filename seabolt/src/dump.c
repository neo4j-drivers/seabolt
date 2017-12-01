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
#include <stdlib.h>

#include "dump.h"


static const char HEX_DIGITS[] = {'0', '1', '2', '3', '4', '5', '6', '7',
                                  '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

#define hex_hi(mem, offset) HEX_DIGITS[((mem)[offset] >> 4) & 0x0F]

#define hex_lo(mem, offset) HEX_DIGITS[(mem)[offset] & 0x0F]

void _bolt_dump_string(char* data, size_t size)
{
    printf("\"");
    for (size_t i = 0; i < size; i++)
    {
        printf("%c", data[i]);
    }
    printf("\"");
}

int bolt_dump_null(const struct BoltValue* value)
{
    assert(value->type == BOLT_NULL);
    printf("~");
    return EXIT_SUCCESS;
}

int bolt_dump_list(const struct BoltValue* value)
{
    assert(value->type == BOLT_LIST);
    printf("[");
    for (int i = 0; i < value->logical_size; i++)
    {
        if (i > 0) printf(", ");
        bolt_dump(bolt_get_list_at(value, i));
    }
    printf("]");
    return EXIT_SUCCESS;
}

int bolt_dump_bit(const struct BoltValue* value)
{
    assert(value->type == BOLT_BIT);
    if (value->is_array)
    {
        printf("b[");
        for (int i = 0; i < value->logical_size; i++)
        {
            printf("%d", bolt_get_bit_array_at(value, i));
        }
        printf("]");
    }
    else
    {
        printf("b(%d)", bolt_get_bit(value));
    }
    return EXIT_SUCCESS;
}

int bolt_dump_byte(const struct BoltValue* value)
{
    assert(value->type == BOLT_BYTE);
    if (value->is_array)
    {
        printf("b8[#");
        for (int i = 0; i < value->logical_size; i++)
        {
            char b = bolt_get_byte_array_at(value, i);
            printf("%c%c", hex_hi(&b, 0), hex_lo(&b, 0));
        }
        printf("]");
    }
    else
    {
        char byte = bolt_get_byte(value);
        printf("b8(#%c%c)", hex_hi(&byte, 0), hex_lo(&byte, 0));
    }
    return EXIT_SUCCESS;
}

int bolt_dump_utf8(const struct BoltValue* value)
{
    assert(value->type == BOLT_UTF8);
    if (value->is_array)
    {
        printf("u8[");
        char* data = value->data.as_char;
        int32_t size;
        for (unsigned long i = 0; i < value->logical_size; i++)
        {
            if (i > 0) { printf(", "); }
            memcpy(&size, data, SIZE_OF_SIZE);
            data += SIZE_OF_SIZE;
            _bolt_dump_string(data, (size_t)(size));
            data += size;
        }
        printf("]");
    }
    else
    {
        printf("u8(\"");
        for (int i = 0; i < value->logical_size; i++)
        {
            printf("%c", value->data.as_char[i]);
        }
        printf("\")");
    }
    return EXIT_SUCCESS;
}

int bolt_dump_utf8_dictionary(const struct BoltValue* value)
{
    assert(value->type == BOLT_UTF8_DICTIONARY);
    printf("d8[");
    int comma = 0;
    for (int i = 0; i < value->logical_size; i++)
    {
        struct BoltValue* key = bolt_get_utf8_dictionary_key_at(value, i);
        if (key != NULL)
        {
            if (comma) printf(", ");
            _bolt_dump_string(bolt_get_utf8(key), (size_t)(key->logical_size));
            printf(" ");
            bolt_dump(bolt_get_utf8_dictionary_at(value, i));
            comma = 1;
        }
    }
    printf("]");
    return EXIT_SUCCESS;
}

int bolt_dump_num8(const struct BoltValue* value)
{
    assert(value->type == BOLT_NUM8);
    if (value->is_array)
    {
        printf("n8[");
        for (int i = 0; i < value->logical_size; i++)
        {
            printf(i == 0 ? "%u" : ", %u", bolt_get_num8_array_at(value, i));
        }
        printf("]");
    }
    else
    {
        printf("n8(%u)", bolt_get_num8(value));
    }
    return EXIT_SUCCESS;
}

int bolt_dump_num16(const struct BoltValue* value)
{
    assert(value->type == BOLT_NUM16);
    if (value->is_array)
    {
        printf("n16[");
        for (int i = 0; i < value->logical_size; i++)
        {
            printf(i == 0 ? "%u" : ", %u", bolt_get_num16_array_at(value, i));
        }
        printf("]");
    }
    else
    {
        printf("n16(%u)", bolt_get_num16(value));
    }
    return EXIT_SUCCESS;
}

int bolt_dump_num32(const struct BoltValue* value)
{
    assert(value->type == BOLT_NUM32);
    if (value->is_array)
    {
        printf("n32[");
        for (int i = 0; i < value->logical_size; i++)
        {
            printf(i == 0 ? "%u" : ", %u", bolt_get_num32_array_at(value, i));
        }
        printf("]");
    }
    else
    {
        printf("n32(%u)", bolt_get_num32(value));
    }
    return EXIT_SUCCESS;
}

int bolt_dump_num64(const struct BoltValue* value)
{
    assert(value->type == BOLT_NUM64);
    if (value->is_array)
    {
        printf("n64[");
        for (int i = 0; i < value->logical_size; i++)
        {
            printf(i == 0 ? "%lu" : ", %lu", bolt_get_num64_array_at(value, i));
        }
        printf("]");
    }
    else
    {
        printf("n64(%lu)", bolt_get_num64(value));
    }
    return EXIT_SUCCESS;
}

int bolt_dump_int8(const struct BoltValue* value)
{
    assert(value->type == BOLT_INT8);
    if (value->is_array)
    {
        printf("i8[");
        for (int i = 0; i < value->logical_size; i++)
        {
            printf(i == 0 ? "%d" : ", %d", bolt_get_int8_array_at(value, i));
        }
        printf("]");
    }
    else
    {
        printf("i8(%d)", bolt_get_int8(value));
    }
    return EXIT_SUCCESS;
}

int bolt_dump_int16(const struct BoltValue* value)
{
    assert(value->type == BOLT_INT16);
    if (value->is_array)
    {
        printf("i16[");
        for (int i = 0; i < value->logical_size; i++)
        {
            printf(i == 0 ? "%d" : ", %d", bolt_get_int16_array_at(value, i));
        }
        printf("]");
    }
    else
    {
        printf("i16(%d)", bolt_get_int16(value));
    }
    return EXIT_SUCCESS;
}

int bolt_dump_int32(const struct BoltValue* value)
{
    assert(value->type == BOLT_INT32);
    if (value->is_array)
    {
        printf("i32[");
        for (int i = 0; i < value->logical_size; i++)
        {
            printf(i == 0 ? "%d" : ", %d", bolt_get_int32_array_at(value, i));
        }
        printf("]");
    }
    else
    {
        printf("i32(%d)", bolt_get_int32(value));
    }
    return EXIT_SUCCESS;
}

int bolt_dump_int64(const struct BoltValue* value)
{
    assert(value->type == BOLT_INT64);
    if (value->is_array)
    {
        printf("i64[");
        for (int i = 0; i < value->logical_size; i++)
        {
            printf(i == 0 ? "%ld" : ", %ld", bolt_get_int64_array_at(value, i));
        }
        printf("]");
    }
    else
    {
        printf("i64(%ld)", bolt_get_int64(value));
    }
    return EXIT_SUCCESS;
}

int bolt_dump_float32(const struct BoltValue* value)
{
    assert(value->type == BOLT_FLOAT32);
    if (value->is_array)
    {
        printf("f32[");
        for (int i = 0; i < value->logical_size; i++)
        {
            printf(i == 0 ? "%f" : ", %f", bolt_get_float32_array_at(value, i));
        }
        printf("]");
    }
    else
    {
        printf("f32(%f)", bolt_get_float32(value));
    }
    return EXIT_SUCCESS;
}

int bolt_dump(struct BoltValue* value)
{
    switch (value->type)
    {
        case BOLT_NULL:
            return bolt_dump_null(value);
        case BOLT_LIST:
            return bolt_dump_list(value);
        case BOLT_BIT:
            return bolt_dump_bit(value);
        case BOLT_BYTE:
            return bolt_dump_byte(value);
        case BOLT_UTF8:
            return bolt_dump_utf8(value);
        case BOLT_UTF8_DICTIONARY:
            return bolt_dump_utf8_dictionary(value);
        case BOLT_NUM8:
            return bolt_dump_num8(value);
        case BOLT_NUM16:
            return bolt_dump_num16(value);
        case BOLT_NUM32:
            return bolt_dump_num32(value);
        case BOLT_NUM64:
            return bolt_dump_num64(value);
        case BOLT_INT8:
            return bolt_dump_int8(value);
        case BOLT_INT16:
            return bolt_dump_int16(value);
        case BOLT_INT32:
            return bolt_dump_int32(value);
        case BOLT_INT64:
            return bolt_dump_int64(value);
        case BOLT_FLOAT32:
            return bolt_dump_float32(value);
        default:
            printf("?");
            return EXIT_FAILURE;
    }
}

void bolt_dump_ln(struct BoltValue* value)
{
    bolt_dump(value);
    printf("\n");
}

