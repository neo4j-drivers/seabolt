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
#include <string.h>
#include <values.h>
#include <assert.h>


void BoltValue_to_Int8(struct BoltValue* value, int8_t x)
{
    _format(value, BOLT_INT8, 1, NULL, 0);
    value->data.as_int8[0] = x;
}

void BoltValue_to_Int16(struct BoltValue* value, int16_t x)
{
    _format(value, BOLT_INT16, 1, NULL, 0);
    value->data.as_int16[0] = x;
}

void BoltValue_to_Int32(struct BoltValue* value, int32_t x)
{
    _format(value, BOLT_INT32, 1, NULL, 0);
    value->data.as_int32[0] = x;
}

void BoltValue_to_Int64(struct BoltValue* value, int64_t x)
{
    _format(value, BOLT_INT64, 1, NULL, 0);
    value->data.as_int64[0] = x;
}

void BoltValue_to_Int8Array(struct BoltValue* value, int8_t* array, int32_t size)
{
    if (size <= sizeof(value->data) / sizeof(int8_t))
    {
        _format(value, BOLT_INT8_ARRAY, size, NULL, 0);
        memcpy(value->data.as_int8, array, (size_t)(size));
    }
    else
    {
        _format(value, BOLT_INT8_ARRAY, size, array, sizeof_n(int8_t, size));
    }
}

void BoltValue_to_Int16Array(struct BoltValue* value, int16_t* array, int32_t size)
{
    if (size <= sizeof(value->data) / sizeof(int16_t))
    {
        _format(value, BOLT_INT16_ARRAY, size, NULL, 0);
        memcpy(value->data.as_int16, array, (size_t)(size));
    }
    else
    {
        _format(value, BOLT_INT16_ARRAY, size, array, sizeof_n(int16_t, size));
    }
}

void BoltValue_to_Int32Array(struct BoltValue* value, int32_t* array, int32_t size)
{
    if (size <= sizeof(value->data) / sizeof(int32_t))
    {
        _format(value, BOLT_INT32_ARRAY, size, NULL, 0);
        memcpy(value->data.as_int32, array, (size_t)(size));
    }
    else
    {
        _format(value, BOLT_INT32_ARRAY, size, array, sizeof_n(int32_t, size));
    }
}

void BoltValue_to_Int64Array(struct BoltValue* value, int64_t* array, int32_t size)
{
    if (size <= sizeof(value->data) / sizeof(int64_t))
    {
        _format(value, BOLT_INT64_ARRAY, size, NULL, 0);
        memcpy(value->data.as_int64, array, (size_t)(size));
    }
    else
    {
        _format(value, BOLT_INT64_ARRAY, size, array, sizeof_n(int64_t, size));
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

int BoltInt8_write(struct BoltValue * value, FILE * file)
{
    assert(BoltValue_type(value) == BOLT_INT8);
    fprintf(file, "i8(%d)", BoltInt8_get(value));
    return 0;
}

int BoltInt16_write(struct BoltValue * value, FILE * file)
{
    assert(BoltValue_type(value) == BOLT_INT16);
    fprintf(file, "i16(%d)", BoltInt16_get(value));
    return 0;
}

int BoltInt32_write(struct BoltValue * value, FILE * file)
{
    assert(BoltValue_type(value) == BOLT_INT32);
    fprintf(file, "i32(%d)", BoltInt32_get(value));
    return 0;
}

int BoltInt64_write(struct BoltValue * value, FILE * file)
{
    assert(BoltValue_type(value) == BOLT_INT64);
    fprintf(file, "i64(%ld)", BoltInt64_get(value));
    return 0;
}

int BoltInt8Array_write(struct BoltValue * value, FILE * file)
{
    assert(BoltValue_type(value) == BOLT_INT8_ARRAY);
    fprintf(file, "i8[");
    for (int i = 0; i < value->size; i++)
    {
        fprintf(file, i == 0 ? "%d" : ", %d", BoltInt8Array_get(value, i));
    }
    fprintf(file, "]");
    return 0;
}

int BoltInt16Array_write(struct BoltValue * value, FILE * file)
{
    assert(BoltValue_type(value) == BOLT_INT16_ARRAY);
    fprintf(file, "i16[");
    for (int i = 0; i < value->size; i++)
    {
        fprintf(file, i == 0 ? "%d" : ", %d", BoltInt16Array_get(value, i));
    }
    fprintf(file, "]");
    return 0;
}

int BoltInt32Array_write(struct BoltValue * value, FILE * file)
{
    assert(BoltValue_type(value) == BOLT_INT32_ARRAY);
    fprintf(file, "i32[");
    for (int i = 0; i < value->size; i++)
    {
        fprintf(file, i == 0 ? "%d" : ", %d", BoltInt32Array_get(value, i));
    }
    fprintf(file, "]");
    return 0;
}

int BoltInt64Array_write(struct BoltValue * value, FILE * file)
{
    assert(BoltValue_type(value) == BOLT_INT64_ARRAY);
    fprintf(file, "i64[");
    for (int i = 0; i < value->size; i++)
    {
        fprintf(file, i == 0 ? "%ld" : ", %ld", BoltInt64Array_get(value, i));
    }
    fprintf(file, "]");
    return 0;
}
