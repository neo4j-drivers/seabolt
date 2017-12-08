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
#include <stdlib.h>

#include "values.h"


static const char HEX_DIGITS[] = {'0', '1', '2', '3', '4', '5', '6', '7',
                                  '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

#define hex3(mem, offset) HEX_DIGITS[((mem)[offset] >> 12) & 0x0F]

#define hex2(mem, offset) HEX_DIGITS[((mem)[offset] >> 8) & 0x0F]

#define hex1(mem, offset) HEX_DIGITS[((mem)[offset] >> 4) & 0x0F]

#define hex0(mem, offset) HEX_DIGITS[(mem)[offset] & 0x0F]

void _bolt_dump_string(const char* data, size_t size)
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
    assert(BoltValue_type(value) == BOLT_NULL);
    printf("~");
    return EXIT_SUCCESS;
}

int BoltBit_dump(const struct BoltValue* value)
{
    assert(BoltValue_type(value) == BOLT_BIT);
    printf("b(%d)", BoltBit_get(value));
    return EXIT_SUCCESS;
}

int BoltBitArray_dump(const struct BoltValue* value)
{
    assert(BoltValue_type(value) == BOLT_BIT_ARRAY);
    printf("b[");
    for (int i = 0; i < value->size; i++)
    {
        printf("%d", BoltBitArray_get(value, i));
    }
    printf("]");
    return EXIT_SUCCESS;
}

int BoltByte_dump(const struct BoltValue* value)
{
    assert(BoltValue_type(value) == BOLT_BYTE);
    char byte = BoltByte_get(value);
    printf("b8(#%c%c)", hex1(&byte, 0), hex0(&byte, 0));
    return EXIT_SUCCESS;
}

int BoltByteArray_dump(const struct BoltValue* value)
{
    assert(BoltValue_type(value) == BOLT_BYTE_ARRAY);
    printf("b8[#");
    for (int i = 0; i < value->size; i++)
    {
        char b = BoltByteArray_get(value, i);
        printf("%c%c", hex1(&b, 0), hex0(&b, 0));
    }
    printf("]");
    return EXIT_SUCCESS;
}

int BoltUTF8_dump(struct BoltValue* value)
{
    assert(BoltValue_type(value) == BOLT_UTF8);
    char* data = BoltUTF8_get(value);
    printf("u8(\"");
    for (int i = 0; i < value->size; i++)
    {
        printf("%c", data[i]);
    }
    printf("\")");
    return EXIT_SUCCESS;
}

int BoltUTF8Array_dump(struct BoltValue* value)
{
    assert(BoltValue_type(value) == BOLT_UTF8_ARRAY);
    printf("u8[");
    for (long i = 0; i < value->size; i++)
    {
        if (i > 0) { printf(", "); }
        struct array_t string = value->data.extended.as_array[i];
        if (string.size == 0)
        {
            printf("\"\"");
        }
        else
        {
            _bolt_dump_string(string.data.as_char, (size_t)(string.size));
        }
    }
    printf("]");
    return EXIT_SUCCESS;
}

int BoltUTF8Dictionary_dump(struct BoltValue* value)
{
    assert(BoltValue_type(value) == BOLT_UTF8_DICTIONARY);
    printf("d8[");
    int comma = 0;
    for (int i = 0; i < value->size; i++)
    {
        struct BoltValue* key = BoltUTF8Dictionary_key(value, i);
        if (key != NULL)
        {
            if (comma) printf(", ");
            _bolt_dump_string(BoltUTF8_get(key), (size_t)(key->size));
            printf(" ");
            BoltValue_dump(BoltUTF8Dictionary_value(value, i));
            comma = 1;
        }
    }
    printf("]");
    return EXIT_SUCCESS;
}

int BoltNum8_dump(struct BoltValue* value)
{
    assert(BoltValue_type(value) == BOLT_NUM8);
    printf("n8(%u)", BoltNum8_get(value));
    return EXIT_SUCCESS;
}

int BoltNum16_dump(struct BoltValue* value)
{
    assert(BoltValue_type(value) == BOLT_NUM16);
    printf("n16(%u)", BoltNum16_get(value));
    return EXIT_SUCCESS;
}

int BoltNum32_dump(struct BoltValue* value)
{
    assert(BoltValue_type(value) == BOLT_NUM32);
    printf("n32(%u)", BoltNum32_get(value));
    return EXIT_SUCCESS;
}

int BoltNum64_dump(struct BoltValue* value)
{
    assert(BoltValue_type(value) == BOLT_NUM64);
    printf("n64(%lu)", BoltNum64_get(value));
    return EXIT_SUCCESS;
}

int BoltNum8Array_dump(struct BoltValue* value)
{
    assert(BoltValue_type(value) == BOLT_NUM8_ARRAY);
    printf("n8[");
    for (int i = 0; i < value->size; i++)
    {
        printf(i == 0 ? "%u" : ", %u", BoltNum8Array_get(value, i));
    }
    printf("]");
    return EXIT_SUCCESS;
}

int BoltNum16Array_dump(struct BoltValue* value)
{
    assert(BoltValue_type(value) == BOLT_NUM16_ARRAY);
    printf("n16[");
    for (int i = 0; i < value->size; i++)
    {
        printf(i == 0 ? "%u" : ", %u", BoltNum16Array_get(value, i));
    }
    printf("]");
    return EXIT_SUCCESS;
}

int BoltNum32Array_dump(struct BoltValue* value)
{
    assert(BoltValue_type(value) == BOLT_NUM32_ARRAY);
    printf("n32[");
    for (int i = 0; i < value->size; i++)
    {
        printf(i == 0 ? "%u" : ", %u", BoltNum32Array_get(value, i));
    }
    printf("]");
    return EXIT_SUCCESS;
}

int BoltNum64Array_dump(struct BoltValue* value)
{
    assert(BoltValue_type(value) == BOLT_NUM64_ARRAY);
    printf("n64[");
    for (int i = 0; i < value->size; i++)
    {
        printf(i == 0 ? "%lu" : ", %lu", BoltNum64Array_get(value, i));
    }
    printf("]");
    return EXIT_SUCCESS;
}

int BoltInt8_dump(struct BoltValue* value)
{
    assert(BoltValue_type(value) == BOLT_INT8);
    printf("i8(%d)", BoltInt8_get(value));
    return EXIT_SUCCESS;
}

int BoltInt16_dump(struct BoltValue* value)
{
    assert(BoltValue_type(value) == BOLT_INT16);
    printf("i16(%d)", BoltInt16_get(value));
    return EXIT_SUCCESS;
}

int BoltInt32_dump(struct BoltValue* value)
{
    assert(BoltValue_type(value) == BOLT_INT32);
    printf("i32(%d)", BoltInt32_get(value));
    return EXIT_SUCCESS;
}

int BoltInt64_dump(struct BoltValue* value)
{
    assert(BoltValue_type(value) == BOLT_INT64);
    printf("i64(%ld)", BoltInt64_get(value));
    return EXIT_SUCCESS;
}

int BoltInt8Array_dump(struct BoltValue* value)
{
    assert(BoltValue_type(value) == BOLT_INT8_ARRAY);
    printf("i8[");
    for (int i = 0; i < value->size; i++)
    {
        printf(i == 0 ? "%d" : ", %d", BoltInt8Array_get(value, i));
    }
    printf("]");
    return EXIT_SUCCESS;
}

int BoltInt16Array_dump(struct BoltValue* value)
{
    assert(BoltValue_type(value) == BOLT_INT16_ARRAY);
    printf("i16[");
    for (int i = 0; i < value->size; i++)
    {
        printf(i == 0 ? "%d" : ", %d", BoltInt16Array_get(value, i));
    }
    printf("]");
    return EXIT_SUCCESS;
}

int BoltInt32Array_dump(struct BoltValue* value)
{
    assert(BoltValue_type(value) == BOLT_INT32_ARRAY);
    printf("i32[");
    for (int i = 0; i < value->size; i++)
    {
        printf(i == 0 ? "%d" : ", %d", BoltInt32Array_get(value, i));
    }
    printf("]");
    return EXIT_SUCCESS;
}

int BoltInt64Array_dump(struct BoltValue* value)
{
    assert(BoltValue_type(value) == BOLT_INT64_ARRAY);
    printf("i64[");
    for (int i = 0; i < value->size; i++)
    {
        printf(i == 0 ? "%ld" : ", %ld", BoltInt64Array_get(value, i));
    }
    printf("]");
    return EXIT_SUCCESS;
}

int BoltFloat32_dump(struct BoltValue* value)
{
    assert(BoltValue_type(value) == BOLT_FLOAT32);
    printf("f32(%f)", BoltFloat32_get(value));
    return EXIT_SUCCESS;
}

int BoltFloat32Array_dump(struct BoltValue* value)
{
    assert(BoltValue_type(value) == BOLT_FLOAT32_ARRAY);
    printf("f32[");
    for (int i = 0; i < value->size; i++)
    {
        printf(i == 0 ? "%f" : ", %f", BoltFloat32Array_get(value, i));
    }
    printf("]");
    return EXIT_SUCCESS;
}

int BoltFloat64_dump(struct BoltValue* value)
{
    assert(BoltValue_type(value) == BOLT_FLOAT64);
    printf("f64(%f)", BoltFloat64_get(value));
    return EXIT_SUCCESS;
}

int BoltFloat64Array_dump(struct BoltValue* value)
{
    assert(BoltValue_type(value) == BOLT_FLOAT64_ARRAY);
    printf("f64[");
    for (int i = 0; i < value->size; i++)
    {
        printf(i == 0 ? "%f" : ", %f", BoltFloat64Array_get(value, i));
    }
    printf("]");
    return EXIT_SUCCESS;
}

int BoltStructure_dump(struct BoltValue* value)
{
    assert(BoltValue_type(value) == BOLT_STRUCTURE);
    int16_t code = BoltStructure_code(value);
    printf("$#%c%c%c%c", hex3(&code, 0), hex2(&code, 0), hex1(&code, 0), hex0(&code, 0));
    printf("(");
    for (int i = 0; i < value->size; i++)
    {
        if (i > 0) printf(" ");
        BoltValue_dump(BoltStructure_value(value, i));
    }
    printf(")");
    return EXIT_SUCCESS;
}

int BoltStructureArray_dump(struct BoltValue* value)
{
    assert(BoltValue_type(value) == BOLT_STRUCTURE_ARRAY);
    int16_t code = BoltStructure_code(value);
    printf("$#%c%c%c%c", hex3(&code, 0), hex2(&code, 0), hex1(&code, 0), hex0(&code, 0));
    printf("[");
    for (int i = 0; i < value->size; i++)
    {
        if (i > 0) printf(", ");
        for (int j = 0; j < BoltStructureArray_getSize(value, i); j++)
        {
            if (j > 0) printf(" ");
            BoltValue_dump(BoltStructureArray_at(value, i, j));
        }
    }
    printf("]");
    return EXIT_SUCCESS;
}

int BoltRequest_dump(struct BoltValue* value)
{
    assert(BoltValue_type(value) == BOLT_REQUEST);
    int16_t code = BoltRequest_code(value);
    switch (code)
    {
        default:
            printf("Request<#%c%c%c%c>", hex3(&code, 0), hex2(&code, 0), hex1(&code, 0), hex0(&code, 0));
    }
    printf("(");
    for (int i = 0; i < value->size; i++)
    {
        if (i > 0) printf(" ");
        BoltValue_dump(BoltRequest_value(value, i));
    }
    printf(")");
    return EXIT_SUCCESS;
}

int BoltSummary_dump(struct BoltValue* value)
{
    assert(BoltValue_type(value) == BOLT_SUMMARY);
    int16_t code = BoltSummary_code(value);
    switch (code)
    {
        default:
            printf("Summary<#%c%c%c%c>", hex3(&code, 0), hex2(&code, 0), hex1(&code, 0), hex0(&code, 0));
    }
    printf("(");
    for (int i = 0; i < value->size; i++)
    {
        if (i > 0) printf(" ");
        BoltValue_dump(BoltSummary_value(value, i));
    }
    printf(")");
    return EXIT_SUCCESS;
}

int BoltList_dump(const struct BoltValue* value)
{
    assert(BoltValue_type(value) == BOLT_LIST);
    printf("[");
    for (int i = 0; i < value->size; i++)
    {
        if (i > 0) printf(", ");
        BoltValue_dump(BoltList_value(value, i));
    }
    printf("]");
    return EXIT_SUCCESS;
}

int BoltValue_dump(struct BoltValue* value)
{
    switch (BoltValue_type(value))
    {
        case BOLT_NULL:
            return BoltNull_dump(value);
        case BOLT_BIT:
            return BoltBit_dump(value);
        case BOLT_BYTE:
            return BoltByte_dump(value);
        case BOLT_BIT_ARRAY:
            return BoltBitArray_dump(value);
        case BOLT_BYTE_ARRAY:
            return BoltByteArray_dump(value);
        case BOLT_UTF8:
            return BoltUTF8_dump(value);
        case BOLT_UTF16:
            return EXIT_FAILURE;
        case BOLT_UTF8_ARRAY:
            return BoltUTF8Array_dump(value);
        case BOLT_UTF16_ARRAY:
            return EXIT_FAILURE;
        case BOLT_UTF8_DICTIONARY:
            return BoltUTF8Dictionary_dump(value);
        case BOLT_UTF16_DICTIONARY:
            return EXIT_FAILURE;
        case BOLT_NUM8:
            return BoltNum8_dump(value);
        case BOLT_NUM16:
            return BoltNum16_dump(value);
        case BOLT_NUM32:
            return BoltNum32_dump(value);
        case BOLT_NUM64:
            return BoltNum64_dump(value);
        case BOLT_NUM8_ARRAY:
            return BoltNum8Array_dump(value);
        case BOLT_NUM16_ARRAY:
            return BoltNum16Array_dump(value);
        case BOLT_NUM32_ARRAY:
            return BoltNum32Array_dump(value);
        case BOLT_NUM64_ARRAY:
            return BoltNum64Array_dump(value);
        case BOLT_INT8:
            return BoltInt8_dump(value);
        case BOLT_INT16:
            return BoltInt16_dump(value);
        case BOLT_INT32:
            return BoltInt32_dump(value);
        case BOLT_INT64:
            return BoltInt64_dump(value);
        case BOLT_INT8_ARRAY:
            return BoltInt8Array_dump(value);
        case BOLT_INT16_ARRAY:
            return BoltInt16Array_dump(value);
        case BOLT_INT32_ARRAY:
            return BoltInt32Array_dump(value);
        case BOLT_INT64_ARRAY:
            return BoltInt64Array_dump(value);
        case BOLT_FLOAT32:
            return BoltFloat32_dump(value);
        case BOLT_FLOAT32_ARRAY:
            return BoltFloat32Array_dump(value);
        case BOLT_FLOAT64:
            return BoltFloat64_dump(value);
        case BOLT_FLOAT64_ARRAY:
            return BoltFloat64Array_dump(value);
        case BOLT_STRUCTURE:
            return BoltStructure_dump(value);
        case BOLT_STRUCTURE_ARRAY:
            return BoltStructureArray_dump(value);
        case BOLT_REQUEST:
            return BoltRequest_dump(value);
        case BOLT_SUMMARY:
            return BoltSummary_dump(value);
        case BOLT_LIST:
            return BoltList_dump(value);
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

