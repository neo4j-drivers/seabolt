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


void _write_string(FILE* file, const char* data, size_t size)
{
    fprintf(file, "\"");
    for (size_t i = 0; i < size; i++)
    {
        fprintf(file, "%c", data[i]);
    }
    fprintf(file, "\"");
}

int BoltNull_write(FILE* file, const struct BoltValue* value)
{
    assert(BoltValue_type(value) == BOLT_NULL);
    fprintf(file, "~");
    return EXIT_SUCCESS;
}

int BoltBit_write(FILE* file, const struct BoltValue* value)
{
    assert(BoltValue_type(value) == BOLT_BIT);
    fprintf(file, "b(%d)", BoltBit_get(value));
    return EXIT_SUCCESS;
}

int BoltBitArray_write(FILE* file, const struct BoltValue* value)
{
    assert(BoltValue_type(value) == BOLT_BIT_ARRAY);
    fprintf(file, "b[");
    for (int i = 0; i < value->size; i++)
    {
        fprintf(file, "%d", BoltBitArray_get(value, i));
    }
    fprintf(file, "]");
    return EXIT_SUCCESS;
}

int BoltByte_write(FILE* file, const struct BoltValue* value)
{
    assert(BoltValue_type(value) == BOLT_BYTE);
    char byte = BoltByte_get(value);
    fprintf(file, "b8(#%c%c)", hex1(&byte, 0), hex0(&byte, 0));
    return EXIT_SUCCESS;
}

int BoltByteArray_write(FILE* file, const struct BoltValue* value)
{
    assert(BoltValue_type(value) == BOLT_BYTE_ARRAY);
    fprintf(file, "b8[#");
    for (int i = 0; i < value->size; i++)
    {
        char b = BoltByteArray_get(value, i);
        fprintf(file, "%c%c", hex1(&b, 0), hex0(&b, 0));
    }
    fprintf(file, "]");
    return EXIT_SUCCESS;
}

int BoltUTF8_write(FILE* file, struct BoltValue* value)
{
    assert(BoltValue_type(value) == BOLT_UTF8);
    char* data = BoltUTF8_get(value);
    fprintf(file, "s8(\"");
    for (int i = 0; i < value->size; i++)
    {
        fprintf(file, "%c", data[i]);
    }
    fprintf(file, "\")");
    return EXIT_SUCCESS;
}

int BoltUTF8Array_write(FILE* file, struct BoltValue* value)
{
    assert(BoltValue_type(value) == BOLT_UTF8_ARRAY);
    fprintf(file, "s8[");
    for (long i = 0; i < value->size; i++)
    {
        if (i > 0) { fprintf(file, ", "); }
        struct array_t string = value->data.extended.as_array[i];
        if (string.size == 0)
        {
            fprintf(file, "\"\"");
        }
        else
        {
            _write_string(file, string.data.as_char, (size_t)(string.size));
        }
    }
    fprintf(file, "]");
    return EXIT_SUCCESS;
}

int BoltUTF8Dictionary_write(FILE* file, struct BoltValue* value)
{
    assert(BoltValue_type(value) == BOLT_UTF8_DICTIONARY);
    fprintf(file, "d8[");
    int comma = 0;
    for (int i = 0; i < value->size; i++)
    {
        struct BoltValue* key = BoltUTF8Dictionary_key(value, i);
        if (key != NULL)
        {
            if (comma) fprintf(file, ", ");
            _write_string(file, BoltUTF8_get(key), (size_t)(key->size));
            fprintf(file, " ");
            BoltValue_write(file, BoltUTF8Dictionary_value(value, i));
            comma = 1;
        }
    }
    fprintf(file, "]");
    return EXIT_SUCCESS;
}

int BoltNum8_write(FILE* file, struct BoltValue* value)
{
    assert(BoltValue_type(value) == BOLT_NUM8);
    fprintf(file, "n8(%u)", BoltNum8_get(value));
    return EXIT_SUCCESS;
}

int BoltNum16_write(FILE* file, struct BoltValue* value)
{
    assert(BoltValue_type(value) == BOLT_NUM16);
    fprintf(file, "n16(%u)", BoltNum16_get(value));
    return EXIT_SUCCESS;
}

int BoltNum32_write(FILE* file, struct BoltValue* value)
{
    assert(BoltValue_type(value) == BOLT_NUM32);
    fprintf(file, "n32(%u)", BoltNum32_get(value));
    return EXIT_SUCCESS;
}

int BoltNum64_write(FILE* file, struct BoltValue* value)
{
    assert(BoltValue_type(value) == BOLT_NUM64);
    fprintf(file, "n64(%lu)", BoltNum64_get(value));
    return EXIT_SUCCESS;
}

int BoltNum8Array_write(FILE* file, struct BoltValue* value)
{
    assert(BoltValue_type(value) == BOLT_NUM8_ARRAY);
    fprintf(file, "n8[");
    for (int i = 0; i < value->size; i++)
    {
        fprintf(file, i == 0 ? "%u" : ", %u", BoltNum8Array_get(value, i));
    }
    fprintf(file, "]");
    return EXIT_SUCCESS;
}

int BoltNum16Array_write(FILE* file, struct BoltValue* value)
{
    assert(BoltValue_type(value) == BOLT_NUM16_ARRAY);
    fprintf(file, "n16[");
    for (int i = 0; i < value->size; i++)
    {
        fprintf(file, i == 0 ? "%u" : ", %u", BoltNum16Array_get(value, i));
    }
    fprintf(file, "]");
    return EXIT_SUCCESS;
}

int BoltNum32Array_write(FILE* file, struct BoltValue* value)
{
    assert(BoltValue_type(value) == BOLT_NUM32_ARRAY);
    fprintf(file, "n32[");
    for (int i = 0; i < value->size; i++)
    {
        fprintf(file, i == 0 ? "%u" : ", %u", BoltNum32Array_get(value, i));
    }
    fprintf(file, "]");
    return EXIT_SUCCESS;
}

int BoltNum64Array_write(FILE* file, struct BoltValue* value)
{
    assert(BoltValue_type(value) == BOLT_NUM64_ARRAY);
    fprintf(file, "n64[");
    for (int i = 0; i < value->size; i++)
    {
        fprintf(file, i == 0 ? "%lu" : ", %lu", BoltNum64Array_get(value, i));
    }
    fprintf(file, "]");
    return EXIT_SUCCESS;
}

int BoltInt8_write(FILE* file, struct BoltValue* value)
{
    assert(BoltValue_type(value) == BOLT_INT8);
    fprintf(file, "i8(%d)", BoltInt8_get(value));
    return EXIT_SUCCESS;
}

int BoltInt16_write(FILE* file, struct BoltValue* value)
{
    assert(BoltValue_type(value) == BOLT_INT16);
    fprintf(file, "i16(%d)", BoltInt16_get(value));
    return EXIT_SUCCESS;
}

int BoltInt32_write(FILE* file, struct BoltValue* value)
{
    assert(BoltValue_type(value) == BOLT_INT32);
    fprintf(file, "i32(%d)", BoltInt32_get(value));
    return EXIT_SUCCESS;
}

int BoltInt64_write(FILE* file, struct BoltValue* value)
{
    assert(BoltValue_type(value) == BOLT_INT64);
    fprintf(file, "i64(%ld)", BoltInt64_get(value));
    return EXIT_SUCCESS;
}

int BoltInt8Array_write(FILE* file, struct BoltValue* value)
{
    assert(BoltValue_type(value) == BOLT_INT8_ARRAY);
    fprintf(file, "i8[");
    for (int i = 0; i < value->size; i++)
    {
        fprintf(file, i == 0 ? "%d" : ", %d", BoltInt8Array_get(value, i));
    }
    fprintf(file, "]");
    return EXIT_SUCCESS;
}

int BoltInt16Array_write(FILE* file, struct BoltValue* value)
{
    assert(BoltValue_type(value) == BOLT_INT16_ARRAY);
    fprintf(file, "i16[");
    for (int i = 0; i < value->size; i++)
    {
        fprintf(file, i == 0 ? "%d" : ", %d", BoltInt16Array_get(value, i));
    }
    fprintf(file, "]");
    return EXIT_SUCCESS;
}

int BoltInt32Array_write(FILE* file, struct BoltValue* value)
{
    assert(BoltValue_type(value) == BOLT_INT32_ARRAY);
    fprintf(file, "i32[");
    for (int i = 0; i < value->size; i++)
    {
        fprintf(file, i == 0 ? "%d" : ", %d", BoltInt32Array_get(value, i));
    }
    fprintf(file, "]");
    return EXIT_SUCCESS;
}

int BoltInt64Array_write(FILE* file, struct BoltValue* value)
{
    assert(BoltValue_type(value) == BOLT_INT64_ARRAY);
    fprintf(file, "i64[");
    for (int i = 0; i < value->size; i++)
    {
        fprintf(file, i == 0 ? "%ld" : ", %ld", BoltInt64Array_get(value, i));
    }
    fprintf(file, "]");
    return EXIT_SUCCESS;
}

int BoltFloat32_write(FILE* file, struct BoltValue* value)
{
    assert(BoltValue_type(value) == BOLT_FLOAT32);
    fprintf(file, "f32(%f)", BoltFloat32_get(value));
    return EXIT_SUCCESS;
}

int BoltFloat32Array_write(FILE* file, struct BoltValue* value)
{
    assert(BoltValue_type(value) == BOLT_FLOAT32_ARRAY);
    fprintf(file, "f32[");
    for (int i = 0; i < value->size; i++)
    {
        fprintf(file, i == 0 ? "%f" : ", %f", BoltFloat32Array_get(value, i));
    }
    fprintf(file, "]");
    return EXIT_SUCCESS;
}

int BoltFloat64_write(FILE* file, struct BoltValue* value)
{
    assert(BoltValue_type(value) == BOLT_FLOAT64);
    fprintf(file, "f64(%f)", BoltFloat64_get(value));
    return EXIT_SUCCESS;
}

int BoltFloat64Array_write(FILE* file, struct BoltValue* value)
{
    assert(BoltValue_type(value) == BOLT_FLOAT64_ARRAY);
    fprintf(file, "f64[");
    for (int i = 0; i < value->size; i++)
    {
        fprintf(file, i == 0 ? "%f" : ", %f", BoltFloat64Array_get(value, i));
    }
    fprintf(file, "]");
    return EXIT_SUCCESS;
}

int BoltStructure_write(FILE* file, struct BoltValue* value)
{
    assert(BoltValue_type(value) == BOLT_STRUCTURE);
    int16_t code = BoltStructure_code(value);
    fprintf(file, "$#%c%c%c%c", hex3(&code, 0), hex2(&code, 0), hex1(&code, 0), hex0(&code, 0));
    fprintf(file, "(");
    for (int i = 0; i < value->size; i++)
    {
        if (i > 0) fprintf(file, " ");
        BoltValue_write(file, BoltStructure_value(value, i));
    }
    fprintf(file, ")");
    return EXIT_SUCCESS;
}

int BoltStructureArray_write(FILE* file, struct BoltValue* value)
{
    assert(BoltValue_type(value) == BOLT_STRUCTURE_ARRAY);
    int16_t code = BoltStructure_code(value);
    fprintf(file, "$#%c%c%c%c", hex3(&code, 0), hex2(&code, 0), hex1(&code, 0), hex0(&code, 0));
    fprintf(file, "[");
    for (int i = 0; i < value->size; i++)
    {
        if (i > 0) fprintf(file, ", ");
        for (int j = 0; j < BoltStructureArray_get_size(value, i); j++)
        {
            if (j > 0) fprintf(file, " ");
            BoltValue_write(file, BoltStructureArray_at(value, i, j));
        }
    }
    fprintf(file, "]");
    return EXIT_SUCCESS;
}

int BoltRequest_write(FILE* file, struct BoltValue* value)
{
    assert(BoltValue_type(value) == BOLT_REQUEST);
    int16_t code = BoltRequest_code(value);
    switch (code)
    {
        default:
            fprintf(file, "Request<#%c%c%c%c>", hex3(&code, 0), hex2(&code, 0), hex1(&code, 0), hex0(&code, 0));
    }
    fprintf(file, "(");
    for (int i = 0; i < value->size; i++)
    {
        if (i > 0) fprintf(file, " ");
        BoltValue_write(file, BoltRequest_value(value, i));
    }
    fprintf(file, ")");
    return EXIT_SUCCESS;
}

int BoltSummary_write(FILE* file, struct BoltValue* value)
{
    assert(BoltValue_type(value) == BOLT_SUMMARY);
    int16_t code = BoltSummary_code(value);
    switch (code)
    {
        default:
            fprintf(file, "Summary<#%c%c%c%c>", hex3(&code, 0), hex2(&code, 0), hex1(&code, 0), hex0(&code, 0));
    }
    fprintf(file, "(");
    for (int i = 0; i < value->size; i++)
    {
        if (i > 0) fprintf(file, " ");
        BoltValue_write(file, BoltSummary_value(value, i));
    }
    fprintf(file, ")");
    return EXIT_SUCCESS;
}

int BoltList_write(FILE* file, const struct BoltValue* value)
{
    assert(BoltValue_type(value) == BOLT_LIST);
    fprintf(file, "[");
    for (int i = 0; i < value->size; i++)
    {
        if (i > 0) fprintf(file, ", ");
        BoltValue_write(file, BoltList_value(value, i));
    }
    fprintf(file, "]");
    return EXIT_SUCCESS;
}

int BoltValue_write(FILE* file, struct BoltValue* value)
{
    switch (BoltValue_type(value))
    {
        case BOLT_NULL:
            return BoltNull_write(file, value);
        case BOLT_BIT:
            return BoltBit_write(file, value);
        case BOLT_BYTE:
            return BoltByte_write(file, value);
        case BOLT_BIT_ARRAY:
            return BoltBitArray_write(file, value);
        case BOLT_BYTE_ARRAY:
            return BoltByteArray_write(file, value);
        case BOLT_UTF8:
            return BoltUTF8_write(file, value);
        case BOLT_UTF16:
            return EXIT_FAILURE;
        case BOLT_UTF8_ARRAY:
            return BoltUTF8Array_write(file, value);
        case BOLT_UTF16_ARRAY:
            return EXIT_FAILURE;
        case BOLT_UTF8_DICTIONARY:
            return BoltUTF8Dictionary_write(file, value);
        case BOLT_UTF16_DICTIONARY:
            return EXIT_FAILURE;
        case BOLT_NUM8:
            return BoltNum8_write(file, value);
        case BOLT_NUM16:
            return BoltNum16_write(file, value);
        case BOLT_NUM32:
            return BoltNum32_write(file, value);
        case BOLT_NUM64:
            return BoltNum64_write(file, value);
        case BOLT_NUM8_ARRAY:
            return BoltNum8Array_write(file, value);
        case BOLT_NUM16_ARRAY:
            return BoltNum16Array_write(file, value);
        case BOLT_NUM32_ARRAY:
            return BoltNum32Array_write(file, value);
        case BOLT_NUM64_ARRAY:
            return BoltNum64Array_write(file, value);
        case BOLT_INT8:
            return BoltInt8_write(file, value);
        case BOLT_INT16:
            return BoltInt16_write(file, value);
        case BOLT_INT32:
            return BoltInt32_write(file, value);
        case BOLT_INT64:
            return BoltInt64_write(file, value);
        case BOLT_INT8_ARRAY:
            return BoltInt8Array_write(file, value);
        case BOLT_INT16_ARRAY:
            return BoltInt16Array_write(file, value);
        case BOLT_INT32_ARRAY:
            return BoltInt32Array_write(file, value);
        case BOLT_INT64_ARRAY:
            return BoltInt64Array_write(file, value);
        case BOLT_FLOAT32:
            return BoltFloat32_write(file, value);
        case BOLT_FLOAT32_ARRAY:
            return BoltFloat32Array_write(file, value);
        case BOLT_FLOAT64:
            return BoltFloat64_write(file, value);
        case BOLT_FLOAT64_ARRAY:
            return BoltFloat64Array_write(file, value);
        case BOLT_STRUCTURE:
            return BoltStructure_write(file, value);
        case BOLT_STRUCTURE_ARRAY:
            return BoltStructureArray_write(file, value);
        case BOLT_REQUEST:
            return BoltRequest_write(file, value);
        case BOLT_SUMMARY:
            return BoltSummary_write(file, value);
        case BOLT_LIST:
            return BoltList_write(file, value);
        default:
            fprintf(file, "?");
            return EXIT_FAILURE;
    }
}
