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

#include <values.h>
#include "_values.h"


void BoltValue_toBit(BoltValue* value, char x)
{
    _BoltValue_to(value, BOLT_BIT, 0, 1, NULL, 0);
    value->data.as_char[0] = x;
}

void BoltValue_toByte(BoltValue* value, char x)
{
    _BoltValue_to(value, BOLT_BYTE, 0, 1, NULL, 0);
    value->data.as_char[0] = x;
}

void BoltValue_toBitArray(BoltValue* value, char* array, int32_t size)
{
    if (size <= sizeof(value->data) / sizeof(char))
    {
        _BoltValue_to(value, BOLT_BIT, 1, size, NULL, 0);
        memcpy(value->data.as_char, array, (size_t)(size));
    }
    else
    {
        _BoltValue_to(value, BOLT_BIT, 1, size, array, sizeof_n(char, size));
    }
}

void BoltValue_toByteArray(BoltValue* value, char* array, int32_t size)
{
    if (size <= sizeof(value->data) / sizeof(char))
    {
        _BoltValue_to(value, BOLT_BYTE, 1, size, NULL, 0);
        memcpy(value->data.as_char, array, (size_t)(size));
    }
    else
    {
        _BoltValue_to(value, BOLT_BYTE, 1, size, array, sizeof_n(char, size));
    }
}

char BoltBit_get(const BoltValue* value)
{
    return to_bit(value->data.as_char[0]);
}

char BoltByte_get(const BoltValue* value)
{
    return value->data.as_char[0];
}

char BoltBitArray_get(const BoltValue* value, int32_t index)
{
    const char* data = value->size <= sizeof(value->data) / sizeof(char) ?
                       value->data.as_char : value->data.extended.as_char;
    return to_bit(data[index]);
}

char BoltByteArray_get(const BoltValue* value, int32_t index)
{
    const char* data = value->size <= sizeof(value->data) / sizeof(char) ?
                       value->data.as_char : value->data.extended.as_char;
    return data[index];
}
