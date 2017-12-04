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



void BoltValue_toChar16(struct BoltValue* value, uint16_t x)
{
    _BoltValue_to(value, BOLT_CHAR16, 0, NULL, 1, 0);
    value->inline_data.as_uint16 = x;
}

void BoltValue_toChar32(struct BoltValue* value, uint32_t x)
{
    _BoltValue_to(value, BOLT_CHAR32, 0, NULL, 1, 0);
    value->inline_data.as_uint32 = x;
}

void BoltValue_toChar16Array(struct BoltValue* value, uint16_t* array, int32_t size)
{
    _BoltValue_to(value, BOLT_CHAR16, 1, array, size, sizeof_n(uint16_t, size));
}

void BoltValue_toChar32Array(struct BoltValue* value, uint32_t* array, int32_t size)
{
    _BoltValue_to(value, BOLT_CHAR32, 1, array, size, sizeof_n(uint32_t, size));
}

void BoltValue_toUTF8(struct BoltValue* value, char* string, int32_t size)
{
    _BoltValue_to(value, BOLT_UTF8, 0, string, size, sizeof_n(char, size));
}

char* BoltUTF8_get(struct BoltValue* value)
{
    return value->data.as_char;
}

void BoltValue_toUTF8Array(struct BoltValue* value, int32_t size)
{
    _BoltValue_to(value, BOLT_UTF8, 1, NULL, size, sizeof_n(struct array_t, size));
    for (int i = 0; i < size; i++)
    {
        value->data.as_array[i].size = 0;
        value->data.as_array[i].data.as_ptr = NULL;
    }
}

void BoltUTF8Array_put(struct BoltValue* value, int32_t index, char* string, int32_t size)
{
    value->data.as_array[index].size = size;
    if (size > 0)
    {
        value->data.as_array[index].data.as_ptr = BoltMem_allocate((size_t)(size));
        memcpy(value->data.as_array[index].data.as_char, string, (size_t)(size));
    }
}

int32_t BoltUTF8Array_getSize(struct BoltValue* value, int32_t index)
{
    return value->data.as_array[index].size;
}

char* BoltUTF8Array_get(struct BoltValue* value, int32_t index)
{
    struct array_t string = value->data.as_array[index];
    return string.size == 0 ? NULL : string.data.as_char;
}

void BoltValue_toUTF8Dictionary(struct BoltValue* value, int32_t size)
{
    size_t unit_size = sizeof(struct BoltValue);
    size_t physical_size = 2 * unit_size * size;
    _BoltValue_recycle(value);
    _BoltValue_allocate(value, physical_size);
    for (size_t offset = 0; offset < physical_size; offset += unit_size)
    {
        _BoltValue_copyData(value, &BOLT_NULL_VALUE, offset, unit_size);
    }
    value->type = BOLT_UTF8_DICTIONARY;
    value->is_array = 0;
    value->logical_size = size;
}

void BoltUTF8Dictionary_resize(struct BoltValue* value, int32_t size)
{
    assert(value->type == BOLT_UTF8_DICTIONARY);
    _BoltValue_resize(value, size, 2);
}

struct BoltValue* BoltUTF8Dictionary_at(const struct BoltValue* value, int32_t index)
{
    assert(value->type == BOLT_UTF8_DICTIONARY);
    return &value->data.as_value[2 * index + 1];
}

struct BoltValue* BoltUTF8Dictionary_withKey(struct BoltValue* value, int32_t index, char* key, int32_t key_size)
{
    assert(value->type == BOLT_UTF8_DICTIONARY);
    BoltValue_toUTF8(&value->data.as_value[2 * index], key, key_size);
    return &value->data.as_value[2 * index + 1];
}

struct BoltValue* BoltUTF8Dictionary_getKey(const struct BoltValue* value, int32_t index)
{
    assert(value->type == BOLT_UTF8_DICTIONARY);
    struct BoltValue* key = &value->data.as_value[2 * index];
    return key->type == BOLT_UTF8 ? key : NULL;
}

void BoltValue_toUTF16(struct BoltValue* value, uint16_t* string, int32_t size)
{
    _BoltValue_to(value, BOLT_UTF16, 0, string, size, sizeof_n(uint16_t, size));
}
