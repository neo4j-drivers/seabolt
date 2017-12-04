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
#include <values.h>

#include "_values.h"



void BoltValue_toChar16(struct BoltValue* value, uint16_t x)
{
    _BoltValue_to(value, BOLT_CHAR16, 0, 1, NULL, 0);
    value->data.as_uint16[0] = x;
}

void BoltValue_toChar32(struct BoltValue* value, uint32_t x)
{
    _BoltValue_to(value, BOLT_CHAR32, 0, 1, NULL, 0);
    value->data.as_uint32[0] = x;
}

void BoltValue_toUTF8(struct BoltValue* value, char* string, int32_t size)
{
    if (size <= sizeof(value->data) / sizeof(char))
    {
        _BoltValue_to(value, BOLT_UTF8, 0, size, NULL, 0);
        memcpy(value->data.as_char, string, (size_t)(size));
    }
    else
    {
        _BoltValue_to(value, BOLT_UTF8, 0, size, string, sizeof_n(char, size));
    }
}

void BoltValue_toUTF16(struct BoltValue* value, uint16_t* string, int32_t size)
{
    if (size <= sizeof(value->data) / sizeof(uint16_t))
    {
        _BoltValue_to(value, BOLT_UTF16, 0, size, NULL, 0);
        memcpy(value->data.as_uint16, string, (size_t)(size));
    }
    else
    {
        _BoltValue_to(value, BOLT_UTF16, 0, size, string, sizeof_n(uint16_t, size));
    }
}

void BoltValue_toChar16Array(struct BoltValue* value, uint16_t* array, int32_t size)
{
    _BoltValue_to(value, BOLT_CHAR16, 1, size, array, sizeof_n(uint16_t, size));
}

void BoltValue_toChar32Array(struct BoltValue* value, uint32_t* array, int32_t size)
{
    _BoltValue_to(value, BOLT_CHAR32, 1, size, array, sizeof_n(uint32_t, size));
}

void BoltValue_toUTF8Array(struct BoltValue* value, int32_t size)
{
    _BoltValue_to(value, BOLT_UTF8, 1, size, NULL, sizeof_n(struct array_t, size));
    for (int i = 0; i < size; i++)
    {
        value->data.extended.as_array[i].size = 0;
        value->data.extended.as_array[i].data.as_ptr = NULL;
    }
}

void BoltValue_toUTF8Dictionary(struct BoltValue* value, int32_t size)
{
    size_t unit_size = sizeof(struct BoltValue);
    size_t data_size = 2 * unit_size * size;
    _BoltValue_recycle(value);
    _BoltValue_allocate(value, data_size);
    memset(value->data.extended.as_char, 0, data_size);
    _BoltValue_setType(value, BOLT_UTF8_DICTIONARY, 0, size);
}

const char* BoltUTF8_get(const struct BoltValue* value)
{
    return value->size <= sizeof(value->data) / sizeof(char) ?
           value->data.as_char : value->data.extended.as_char;
}

char* BoltUTF8Array_get(struct BoltValue* value, int32_t index)
{
    struct array_t string = value->data.extended.as_array[index];
    return string.size == 0 ? NULL : string.data.as_char;
}

int32_t BoltUTF8Array_getSize(struct BoltValue* value, int32_t index)
{
    return value->data.extended.as_array[index].size;
}

void BoltUTF8Array_put(struct BoltValue* value, int32_t index, char* string, int32_t size)
{
    value->data.extended.as_array[index].size = size;
    if (size > 0)
    {
        value->data.extended.as_array[index].data.as_ptr = BoltMem_allocate((size_t)(size));
        memcpy(value->data.extended.as_array[index].data.as_char, string, (size_t)(size));
    }
}

struct BoltValue* BoltUTF8Dictionary_getKey(const struct BoltValue* value, int32_t index)
{
    assert(BoltValue_type(value) == BOLT_UTF8_DICTIONARY);
    struct BoltValue* key = &value->data.extended.as_value[2 * index];
    return BoltValue_type(key) == BOLT_UTF8 ? key : NULL;
}

struct BoltValue* BoltUTF8Dictionary_withKey(struct BoltValue* value, int32_t index, char* key, int32_t key_size)
{
    assert(BoltValue_type(value) == BOLT_UTF8_DICTIONARY);
    BoltValue_toUTF8(&value->data.extended.as_value[2 * index], key, key_size);
    return &value->data.extended.as_value[2 * index + 1];
}

struct BoltValue* BoltUTF8Dictionary_at(const struct BoltValue* value, int32_t index)
{
    assert(BoltValue_type(value) == BOLT_UTF8_DICTIONARY);
    return &value->data.extended.as_value[2 * index + 1];
}

void BoltUTF8Dictionary_resize(struct BoltValue* value, int32_t size)
{
    assert(BoltValue_type(value) == BOLT_UTF8_DICTIONARY);
    _BoltValue_resize(value, size, 2);
}
