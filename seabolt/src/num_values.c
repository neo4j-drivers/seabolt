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

#include "_values.h"


void BoltValue_toNum8(BoltValue* value, uint8_t x)
{
    _BoltValue_to(value, BOLT_NUM8, 0, 1, NULL, 0);
    value->data.as_uint8[0] = x;
}

void BoltValue_toNum16(BoltValue* value, uint16_t x)
{
    _BoltValue_to(value, BOLT_NUM16, 0, 1, NULL, 0);
    value->data.as_uint16[0] = x;
}

void BoltValue_toNum32(BoltValue* value, uint32_t x)
{
    _BoltValue_to(value, BOLT_NUM32, 0, 1, NULL, 0);
    value->data.as_uint32[0] = x;
}

void BoltValue_toNum64(BoltValue* value, uint64_t x)
{
    _BoltValue_to(value, BOLT_NUM64, 0, 1, NULL, 0);
    value->data.as_uint64[0] = x;
}

void BoltValue_toNum8Array(BoltValue* value, uint8_t* array, int32_t size)
{
    if (size <= sizeof(value->data) / sizeof(uint8_t))
    {
        _BoltValue_to(value, BOLT_NUM8, 1, size, NULL, 0);
        memcpy(value->data.as_uint8, array, (size_t)(size));
    }
    else
    {
        _BoltValue_to(value, BOLT_NUM8, 1, size, array, sizeof_n(uint8_t, size));
    }
}

void BoltValue_toNum16Array(BoltValue* value, uint16_t* array, int32_t size)
{
    if (size <= sizeof(value->data) / sizeof(uint16_t))
    {
        _BoltValue_to(value, BOLT_NUM16, 1, size, NULL, 0);
        memcpy(value->data.as_uint16, array, (size_t)(size));
    }
    else
    {
        _BoltValue_to(value, BOLT_NUM16, 1, size, array, sizeof_n(uint16_t, size));
    }
}

void BoltValue_toNum32Array(BoltValue* value, uint32_t* array, int32_t size)
{
    if (size <= sizeof(value->data) / sizeof(uint32_t))
    {
        _BoltValue_to(value, BOLT_NUM32, 1, size, NULL, 0);
        memcpy(value->data.as_uint32, array, (size_t)(size));
    }
    else
    {
        _BoltValue_to(value, BOLT_NUM32, 1, size, array, sizeof_n(uint32_t, size));
    }
}

void BoltValue_toNum64Array(BoltValue* value, uint64_t* array, int32_t size)
{
    if (size <= sizeof(value->data) / sizeof(uint64_t))
    {
        _BoltValue_to(value, BOLT_NUM64, 1, size, NULL, 0);
        memcpy(value->data.as_uint64, array, (size_t)(size));
    }
    else
    {
        _BoltValue_to(value, BOLT_NUM64, 1, size, array, sizeof_n(uint64_t, size));
    }
}

uint8_t BoltNum8_get(const BoltValue* value)
{
    return value->data.as_uint8[0];
}

uint16_t BoltNum16_get(const BoltValue* value)
{
    return value->data.as_uint16[0];
}

uint32_t BoltNum32_get(const BoltValue* value)
{
    return value->data.as_uint32[0];
}

uint64_t BoltNum64_get(const BoltValue* value)
{
    return value->data.as_uint64[0];
}

uint8_t BoltNum8Array_get(const BoltValue* value, int32_t index)
{
    const uint8_t* data = value->size <= sizeof(value->data) / sizeof(uint8_t) ?
                          value->data.as_uint8 : value->data.extended.as_uint8;
    return data[index];
}

uint16_t BoltNum16Array_get(const BoltValue* value, int32_t index)
{
    const uint16_t* data = value->size <= sizeof(value->data) / sizeof(uint16_t) ?
                           value->data.as_uint16 : value->data.extended.as_uint16;
    return data[index];
}

uint32_t BoltNum32Array_get(const BoltValue* value, int32_t index)
{
    const uint32_t* data = value->size <= sizeof(value->data) / sizeof(uint32_t) ?
                           value->data.as_uint32 : value->data.extended.as_uint32;
    return data[index];
}

uint64_t BoltNum64Array_get(const BoltValue* value, int32_t index)
{
    const uint64_t* data = value->size <= sizeof(value->data) / sizeof(uint64_t) ?
                           value->data.as_uint64 : value->data.extended.as_uint64;
    return data[index];
}
