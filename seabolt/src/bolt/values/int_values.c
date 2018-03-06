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

#include <stdint.h>
#include <string.h>
#include <bolt/values.h>
#include <assert.h>
#include <inttypes.h>
#include "bolt/mem.h"


void BoltValue_to_Int16(struct BoltValue* value, int16_t data)
{
    _format(value, BOLT_INT16, 0, 1, NULL, 0);
    value->data.as_int16[0] = data;
}

void BoltValue_to_Int32(struct BoltValue* value, int32_t data)
{
    _format(value, BOLT_INT32, 0, 1, NULL, 0);
    value->data.as_int32[0] = data;
}

void BoltValue_to_Int64(struct BoltValue* value, int64_t data)
{
    _format(value, BOLT_INT64, 0, 1, NULL, 0);
    value->data.as_int64[0] = data;
}

void BoltValue_to_Int16Array(struct BoltValue* value, int16_t* data, int32_t length)
{
    if (length <= sizeof(value->data) / sizeof(int16_t))
    {
        _format(value, BOLT_INT16_ARRAY, 0, length, NULL, 0);
        memcpy(value->data.as_int16, data, (size_t)(length));
    }
    else
    {
        _format(value, BOLT_INT16_ARRAY, 0, length, data, sizeof_n(int16_t, length));
    }
}

void BoltValue_to_Int32Array(struct BoltValue* value, int32_t* data, int32_t length)
{
    if (length <= sizeof(value->data) / sizeof(int32_t))
    {
        _format(value, BOLT_INT32_ARRAY, 0, length, NULL, 0);
        memcpy(value->data.as_int32, data, (size_t)(length));
    }
    else
    {
        _format(value, BOLT_INT32_ARRAY, 0, length, data, sizeof_n(int32_t, length));
    }
}

void BoltValue_to_Int64Array(struct BoltValue* value, int64_t* data, int32_t length)
{
    if (length <= sizeof(value->data) / sizeof(int64_t))
    {
        _format(value, BOLT_INT64_ARRAY, 0, length, NULL, 0);
        memcpy(value->data.as_int64, data, (size_t)(length));
    }
    else
    {
        _format(value, BOLT_INT64_ARRAY, 0, length, data, sizeof_n(int64_t, length));
    }
}

int8_t BoltInt8_get(const struct BoltValue* value)
{
    return value->data.as_int8[0];
}

int16_t BoltInt16_get(const struct BoltValue* value)
{
    return value->data.as_int16[0];
}

int32_t BoltInt32_get(const struct BoltValue* value)
{
    return value->data.as_int32[0];
}

int64_t BoltInt64_get(const struct BoltValue* value)
{
    return value->data.as_int64[0];
}

int8_t BoltInt8Array_get(const struct BoltValue* value, int32_t index)
{
    const int8_t* data = value->size <= sizeof(value->data) / sizeof(int8_t) ?
                         value->data.as_int8 : value->data.extended.as_int8;
    return data[index];
}

int16_t BoltInt16Array_get(const struct BoltValue* value, int32_t index)
{
    const int16_t* data = value->size <= sizeof(value->data) / sizeof(int16_t) ?
                          value->data.as_int16 : value->data.extended.as_int16;
    return data[index];
}

int32_t BoltInt32Array_get(const struct BoltValue* value, int32_t index)
{
    const int32_t* data = value->size <= sizeof(value->data) / sizeof(int32_t) ?
                          value->data.as_int32 : value->data.extended.as_int32;
    return data[index];
}

int64_t BoltInt64Array_get(const struct BoltValue* value, int32_t index)
{
    const int64_t* data = value->size <= sizeof(value->data) / sizeof(int64_t) ?
                          value->data.as_int64 : value->data.extended.as_int64;
    return data[index];
}

int BoltInt16_write(struct BoltValue * value, FILE * file)
{
    assert(BoltValue_type(value) == BOLT_INT16);
    fprintf(file, "%" PRIi16 "s", BoltInt16_get(value));
    return 0;
}

int BoltInt32_write(struct BoltValue * value, FILE * file)
{
    assert(BoltValue_type(value) == BOLT_INT32);
    fprintf(file, "%" PRIi32, BoltInt32_get(value));
    return 0;
}

int BoltInt64_write(struct BoltValue * value, FILE * file)
{
    assert(BoltValue_type(value) == BOLT_INT64);
    fprintf(file, "%" PRIi64 "L", BoltInt64_get(value));
    return 0;
}

int BoltInt16Array_write(struct BoltValue * value, FILE * file)
{
    assert(BoltValue_type(value) == BOLT_INT16_ARRAY);
    fprintf(file, "s[");
    for (int i = 0; i < value->size; i++)
    {
        fprintf(file, i == 0 ? "%" PRId16 : ", %" PRId16, BoltInt16Array_get(value, i));
    }
    fprintf(file, "]");
    return 0;
}

int BoltInt32Array_write(struct BoltValue * value, FILE * file)
{
    assert(BoltValue_type(value) == BOLT_INT32_ARRAY);
    fprintf(file, "_[");
    for (int i = 0; i < value->size; i++)
    {
        fprintf(file, i == 0 ? "%" PRId32 : ", %" PRId32, BoltInt32Array_get(value, i));
    }
    fprintf(file, "]");
    return 0;
}

int BoltInt64Array_write(struct BoltValue * value, FILE * file)
{
    assert(BoltValue_type(value) == BOLT_INT64_ARRAY);
    fprintf(file, "L[");
    for (int i = 0; i < value->size; i++)
    {
        fprintf(file, i == 0 ? "%" PRId64 : ", %" PRId64, BoltInt64Array_get(value, i));
    }
    fprintf(file, "]");
    return 0;
}
