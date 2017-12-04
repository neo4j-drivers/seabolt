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

#include "_values.h"


void BoltValue_toBit(struct BoltValue* value, char x)
{
    _BoltValue_to(value, BOLT_BIT, 0, &x, 1, sizeof(char));
}

char BoltBit_get(const struct BoltValue* value)
{
    return to_bit(value->data.as_char[0]);
}

void BoltValue_toBitArray(struct BoltValue* value, char* array, int32_t size)
{
    _BoltValue_to(value, BOLT_BIT, 1, array, size, sizeof_n(char, size));
}

char BoltBitArray_get(const struct BoltValue* value, int32_t index)
{
    return to_bit(value->data.as_char[index]);
}

void BoltValue_toByte(struct BoltValue* value, char x)
{
    _BoltValue_to(value, BOLT_BYTE, 0, &x, 1, sizeof(char));
}

char BoltByte_get(const struct BoltValue* value)
{
    return value->data.as_char[0];
}

void BoltValue_toByteArray(struct BoltValue* value, char* array, int32_t size)
{
    _BoltValue_to(value, BOLT_BYTE, 1, array, size, sizeof_n(char, size));
}

char BoltByteArray_get(const struct BoltValue* value, int32_t index)
{
    return value->data.as_char[index];
}
