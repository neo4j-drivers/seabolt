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


#include <assert.h>
#include <stdint.h>
#include <memory.h>
#include <values.h>


void _copyData(struct BoltValue* value, const void* data, size_t offset, size_t length)
{
    if (length > 0)
    {
        memcpy(value->data.extended.as_char + offset, data, length);
    }
}

/**
 * Clean up a value for reuse.
 *
 * This sets any nested values to null.
 *
 * @param value
 */
void _recycle(struct BoltValue* value)
{
    BoltType type = BoltValue_type(value);
    if (type == BOLT_LIST || type == BOLT_STRUCTURE || type == BOLT_REQUEST || type == BOLT_SUMMARY)
    {
        for (long i = 0; i < value->size; i++)
        {
            BoltValue_toNull(&value->data.extended.as_value[i]);
        }
    }
    else if (type == BOLT_UTF8 && BoltValue_isArray(value))
    {
        for (long i = 0; i < value->size; i++)
        {
            struct array_t string = value->data.extended.as_array[i];
            if (string.size > 0)
            {
                BoltMem_deallocate(string.data.as_ptr, (size_t)(string.size));
            }
        }
    }
    else if (type == BOLT_UTF8_DICTIONARY)
    {
        for (long i = 0; i < 2 * value->size; i++)
        {
            BoltValue_toNull(&value->data.extended.as_value[i]);
        }
    }
    else if (type == BOLT_UTF16_DICTIONARY)
    {
        // TODO
    }
}

void _setType(struct BoltValue* value, BoltType type, char is_array, int size)
{
    assert(type < 0x80);
    value->type = (char)(type);
    value->is_array = is_array;
    value->size = size;
}

void _format(struct BoltValue* value, BoltType type, char is_array, int size, const void* data, size_t data_size)
{
    _recycle(value);
    value->data.extended.as_ptr = BoltMem_adjust(value->data.extended.as_ptr, value->data_size, data_size);
    value->data_size = data_size;
    if (data != NULL)
    {
        _copyData(value, data, 0, data_size);
    }
    _setType(value, type, is_array, size);
}


/**
 * Resize a value that contains multiple sub-values.
 *
 * @param value
 * @param size
 * @param multiplier
 */
void _resize(struct BoltValue* value, int32_t size, int multiplier)
{
    if (size > value->size)
    {
        // grow physically
        size_t unit_size = sizeof(struct BoltValue);
        size_t new_data_size = multiplier * unit_size * size;
        size_t old_data_size = value->data_size;
        value->data.extended.as_ptr = BoltMem_adjust(value->data.extended.as_ptr, value->data_size, new_data_size);
        value->data_size = new_data_size;
        // grow logically
        memset(value->data.extended.as_char + old_data_size, 0, new_data_size - old_data_size);
        value->size = size;
    }
    else if (size < value->size)
    {
        // shrink logically
        for (long i = multiplier * size; i < multiplier * value->size; i++)
        {
            BoltValue_toNull(&value->data.extended.as_value[i]);
        }
        value->size = size;
        // shrink physically
        size_t new_data_size = multiplier * sizeof_n(struct BoltValue, size);
        value->data.extended.as_ptr = BoltMem_adjust(value->data.extended.as_ptr, value->data_size, new_data_size);
        value->data_size = new_data_size;
    }
    else
    {
        // same size - do nothing
    }
}

/**
 * Create a new `BoltValue` instance.
 *
 * @return
 */
struct BoltValue* BoltValue_create()
{
    size_t size = sizeof(struct BoltValue);
    struct BoltValue* value = BoltMem_allocate(size);
    _setType(value, BOLT_NULL, 0, 0);
    value->data_size = 0;
    value->data.as_uint64[0] = 0;
    value->data.extended.as_ptr = NULL;
    return value;
}

void BoltValue_toNull(struct BoltValue* value)
{
    if (BoltValue_type(value) != BOLT_NULL)
    {
        _format(value, BOLT_NULL, 0, 0, NULL, 0);
    }
}

void BoltValue_toList(struct BoltValue* value, int32_t size)
{
    if (BoltValue_type(value) == BOLT_LIST)
    {
        BoltList_resize(value, size);
    }
    else
    {
        size_t data_size = sizeof(struct BoltValue) * size;
        _recycle(value);
        value->data.extended.as_ptr = BoltMem_adjust(value->data.extended.as_ptr, value->data_size, data_size);
        value->data_size = data_size;
        memset(value->data.extended.as_char, 0, data_size);
        _setType(value, BOLT_LIST, 0, size);
    }
}

BoltType BoltValue_type(const struct BoltValue* value)
{
    return (BoltType)(value->type);
}

int BoltValue_isArray(const struct BoltValue* value)
{
    return value->is_array;
}

void BoltValue_destroy(struct BoltValue* value)
{
    BoltValue_toNull(value);
    BoltMem_deallocate(value, sizeof(struct BoltValue));
}

void BoltList_resize(struct BoltValue* value, int32_t size)
{
    assert(BoltValue_type(value) == BOLT_LIST);
    _resize(value, size, 1);
}

struct BoltValue* BoltList_value(const struct BoltValue* value, int32_t index)
{
    assert(BoltValue_type(value) == BOLT_LIST);
    return &value->data.extended.as_value[index];
}
