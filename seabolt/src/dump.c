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

#define hex3(mem, offset) HEX_DIGITS[((mem)[offset] >> 12) & 0x0F]

#define hex2(mem, offset) HEX_DIGITS[((mem)[offset] >> 8) & 0x0F]

#define hex1(mem, offset) HEX_DIGITS[((mem)[offset] >> 4) & 0x0F]

#define hex0(mem, offset) HEX_DIGITS[(mem)[offset] & 0x0F]

void _bolt_dump_string(char* data, size_t size)
{
    printf("\"");
    for (size_t i = 0; i < size; i++)
    {
        printf("%c", data[i]);
    }
    printf("\"");
}

int BoltNull_dump(const struct BoltValue* value)
{
    assert(value->type == BOLT_NULL);
    printf("~");
    return EXIT_SUCCESS;
}

int BoltList_dump(const struct BoltValue* value)
{
    assert(value->type == BOLT_LIST);
    printf("[");
    for (int i = 0; i < value->logical_size; i++)
    {
        if (i > 0) printf(", ");
        BoltValue_dump(BoltList_at(value, i));
    }
    printf("]");
    return EXIT_SUCCESS;
}

int BoltBit_dump(const struct BoltValue* value)
{
    assert(value->type == BOLT_BIT);
    if (value->is_array)
    {
        printf("b[");
        for (int i = 0; i < value->logical_size; i++)
        {
            printf("%d", BoltBitArray_get(value, i));
        }
        printf("]");
    }
    else
    {
        printf("b(%d)", BoltBit_get(value));
    }
    return EXIT_SUCCESS;
}

int BoltByte_dump(const struct BoltValue* value)
{
    assert(value->type == BOLT_BYTE);
    if (value->is_array)
    {
        printf("b8[#");
        for (int i = 0; i < value->logical_size; i++)
        {
            char b = BoltByteArray_get(value, i);
            printf("%c%c", hex1(&b, 0), hex0(&b, 0));
        }
        printf("]");
    }
    else
    {
        char byte = BoltByte_get(value);
        printf("b8(#%c%c)", hex1(&byte, 0), hex0(&byte, 0));
    }
    return EXIT_SUCCESS;
}

int BoltUTF8_dump(const struct BoltValue* value)
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

int BoltUTF8Dictionary_dump(const struct BoltValue* value)
{
    assert(value->type == BOLT_UTF8_DICTIONARY);
    printf("d8[");
    int comma = 0;
    for (int i = 0; i < value->logical_size; i++)
    {
        struct BoltValue* key = BoltUTF8Dictionary_getKey(value, i);
        if (key != NULL)
        {
            if (comma) printf(", ");
            _bolt_dump_string(BoltUTF8_get(key), (size_t)(key->logical_size));
            printf(" ");
            BoltValue_dump(BoltUTF8Dictionary_at(value, i));
            comma = 1;
        }
    }
    printf("]");
    return EXIT_SUCCESS;
}

int BoltNum8_dump(const struct BoltValue* value)
{
    assert(value->type == BOLT_NUM8);
    if (value->is_array)
    {
        printf("n8[");
        for (int i = 0; i < value->logical_size; i++)
        {
            printf(i == 0 ? "%u" : ", %u", BoltNum8Array_get(value, i));
        }
        printf("]");
    }
    else
    {
        printf("n8(%u)", BoltNum8_get(value));
    }
    return EXIT_SUCCESS;
}

int BoltNum16_dump(const struct BoltValue* value)
{
    assert(value->type == BOLT_NUM16);
    if (value->is_array)
    {
        printf("n16[");
        for (int i = 0; i < value->logical_size; i++)
        {
            printf(i == 0 ? "%u" : ", %u", BoltNum16Array_get(value, i));
        }
        printf("]");
    }
    else
    {
        printf("n16(%u)", BoltNum16_get(value));
    }
    return EXIT_SUCCESS;
}

int BoltNum32_dump(const struct BoltValue* value)
{
    assert(value->type == BOLT_NUM32);
    if (value->is_array)
    {
        printf("n32[");
        for (int i = 0; i < value->logical_size; i++)
        {
            printf(i == 0 ? "%u" : ", %u", BoltNum32Array_get(value, i));
        }
        printf("]");
    }
    else
    {
        printf("n32(%u)", BoltNum32_get(value));
    }
    return EXIT_SUCCESS;
}

int BoltNum64_dump(const struct BoltValue* value)
{
    assert(value->type == BOLT_NUM64);
    if (value->is_array)
    {
        printf("n64[");
        for (int i = 0; i < value->logical_size; i++)
        {
            printf(i == 0 ? "%lu" : ", %lu", BoltNum64Array_get(value, i));
        }
        printf("]");
    }
    else
    {
        printf("n64(%lu)", BoltNum64_get(value));
    }
    return EXIT_SUCCESS;
}

int BoltInt8_dump(const struct BoltValue* value)
{
    assert(value->type == BOLT_INT8);
    if (value->is_array)
    {
        printf("i8[");
        for (int i = 0; i < value->logical_size; i++)
        {
            printf(i == 0 ? "%d" : ", %d", BoltInt8Array_get(value, i));
        }
        printf("]");
    }
    else
    {
        printf("i8(%d)", BoltInt8_get(value));
    }
    return EXIT_SUCCESS;
}

int BoltInt16_dump(const struct BoltValue* value)
{
    assert(value->type == BOLT_INT16);
    if (value->is_array)
    {
        printf("i16[");
        for (int i = 0; i < value->logical_size; i++)
        {
            printf(i == 0 ? "%d" : ", %d", BoltInt16Array_get(value, i));
        }
        printf("]");
    }
    else
    {
        printf("i16(%d)", BoltInt16_get(value));
    }
    return EXIT_SUCCESS;
}

int BoltInt32_dump(const struct BoltValue* value)
{
    assert(value->type == BOLT_INT32);
    if (value->is_array)
    {
        printf("i32[");
        for (int i = 0; i < value->logical_size; i++)
        {
            printf(i == 0 ? "%d" : ", %d", BoltInt32Array_get(value, i));
        }
        printf("]");
    }
    else
    {
        printf("i32(%d)", BoltInt32_get(value));
    }
    return EXIT_SUCCESS;
}

int BoltInt64_dump(const struct BoltValue* value)
{
    assert(value->type == BOLT_INT64);
    if (value->is_array)
    {
        printf("i64[");
        for (int i = 0; i < value->logical_size; i++)
        {
            printf(i == 0 ? "%ld" : ", %ld", BoltInt64Array_get(value, i));
        }
        printf("]");
    }
    else
    {
        printf("i64(%ld)", BoltInt64_get(value));
    }
    return EXIT_SUCCESS;
}

int BoltFloat32_dump(const struct BoltValue* value)
{
    assert(value->type == BOLT_FLOAT32);
    if (value->is_array)
    {
        printf("f32[");
        for (int i = 0; i < value->logical_size; i++)
        {
            printf(i == 0 ? "%f" : ", %f", BoltFloat32Array_get(value, i));
        }
        printf("]");
    }
    else
    {
        printf("f32(%f)", BoltFloat32_get(value));
    }
    return EXIT_SUCCESS;
}

int BoltStructure_dump(const struct BoltValue* value)
{
    assert(value->type == BOLT_STRUCTURE);
    switch (value->code)
    {
        case 0xA0:
            printf("$Node");
            break;
        default:
            printf("$#%c%c%c%c", hex3(&value->code, 0), hex2(&value->code, 0), hex1(&value->code, 0), hex0(&value->code, 0));
    }
    printf("(");
    for (int i = 0; i < value->logical_size; i++)
    {
        if (i > 0) printf(" ");
        BoltValue_dump(BoltStructure_at(value, i));
    }
    printf(")");
    return EXIT_SUCCESS;
}

int BoltValue_dump(struct BoltValue* value)
{
    switch (value->type)
    {
        case BOLT_NULL:
            return BoltNull_dump(value);
        case BOLT_LIST:
            return BoltList_dump(value);
        case BOLT_BIT:
            return BoltBit_dump(value);
        case BOLT_BYTE:
            return BoltByte_dump(value);
        case BOLT_UTF8:
            return BoltUTF8_dump(value);
        case BOLT_UTF8_DICTIONARY:
            return BoltUTF8Dictionary_dump(value);
        case BOLT_NUM8:
            return BoltNum8_dump(value);
        case BOLT_NUM16:
            return BoltNum16_dump(value);
        case BOLT_NUM32:
            return BoltNum32_dump(value);
        case BOLT_NUM64:
            return BoltNum64_dump(value);
        case BOLT_INT8:
            return BoltInt8_dump(value);
        case BOLT_INT16:
            return BoltInt16_dump(value);
        case BOLT_INT32:
            return BoltInt32_dump(value);
        case BOLT_INT64:
            return BoltInt64_dump(value);
        case BOLT_FLOAT32:
            return BoltFloat32_dump(value);
        case BOLT_STRUCTURE:
            return BoltStructure_dump(value);
        default:
            printf("?");
            return EXIT_FAILURE;
    }
}

void BoltValue_dumpLine(struct BoltValue* value)
{
    BoltValue_dump(value);
    printf("\n");
}

