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


/*********************************************************************
 * Num8 / Num8Array
 */

void BoltValue_toNum8(struct BoltValue* value, uint8_t x)
{
    _BoltValue_to(value, BOLT_NUM8, 0, 1, NULL, 0);
    value->data.as_uint8[0] = x;
}

uint8_t BoltNum8_get(const struct BoltValue* value)
{
    return value->data.as_uint8[0];
}

void BoltValue_toNum8Array(struct BoltValue* value, uint8_t* array, int32_t size)
{
    _BoltValue_to(value, BOLT_NUM8, 1, size, array, sizeof_n(uint8_t, size));
}

uint8_t BoltNum8Array_get(const struct BoltValue* value, int32_t index)
{
    return value->data.extended.as_uint8[index];
}


/*********************************************************************
 * Num16 / Num16Array
 */

void BoltValue_toNum16(struct BoltValue* value, uint16_t x)
{
    _BoltValue_to(value, BOLT_NUM16, 0, 1, NULL, 0);
    value->data.as_uint16[0] = x;
}

uint16_t BoltNum16_get(const struct BoltValue* value)
{
    return value->data.as_uint16[0];
}

void BoltValue_toNum16Array(struct BoltValue* value, uint16_t* array, int32_t size)
{
    _BoltValue_to(value, BOLT_NUM16, 1, size, array, sizeof_n(uint16_t, size));
}

uint16_t BoltNum16Array_get(const struct BoltValue* value, int32_t index)
{
    return value->data.extended.as_uint16[index];
}


/*********************************************************************
 * Num32 / Num32Array
 */

void BoltValue_toNum32(struct BoltValue* value, uint32_t x)
{
    _BoltValue_to(value, BOLT_NUM32, 0, 1, NULL, 0);
    value->data.as_uint32[0] = x;
}

uint32_t BoltNum32_get(const struct BoltValue* value)
{
    return value->data.as_uint32[0];
}

void BoltValue_toNum32Array(struct BoltValue* value, uint32_t* array, int32_t size)
{
    _BoltValue_to(value, BOLT_NUM32, 1, size, array, sizeof_n(uint32_t, size));
}

uint32_t BoltNum32Array_get(const struct BoltValue* value, int32_t index)
{
    return value->data.extended.as_uint32[index];
}


/*********************************************************************
 * Num64 / Num64Array
 */

void BoltValue_toNum64(struct BoltValue* value, uint64_t x)
{
    _BoltValue_to(value, BOLT_NUM64, 0, 1, NULL, 0);
    value->data.as_uint64[0] = x;
}

uint64_t BoltNum64_get(const struct BoltValue* value)
{
    return value->data.as_uint64[0];
}

void BoltValue_toNum64Array(struct BoltValue* value, uint64_t* array, int32_t size)
{
    _BoltValue_to(value, BOLT_NUM64, 1, size, array, sizeof_n(uint64_t, size));
}

uint64_t BoltNum64Array_get(const struct BoltValue* value, int32_t index)
{
    return value->data.extended.as_uint64[index];
}
