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


/**
 * Allocate, reallocate or free memory for data storage.
 *
 * Since we recycle values, we can also potentially recycle
 * the dynamically-allocated storage.
 *
 * @param value the value in which to allocate storage
 * @param data_size the number of bytes of storage required
 */
void _BoltValue_allocate(struct BoltValue* value, size_t data_size)
{
    if (data_size == value->data_size)
    {
        // In this case, the physical data storage requirement
        // hasn't changed, whether zero or some positive value.
        // This means that we can reuse the storage exactly
        // as-is and no allocation needs to occur. Therefore,
        // if we reuse a value for the same fixed-size type -
        // an int32 for example - then we can continue to reuse
        // the same storage for each value, avoiding memory
        // reallocation.
    }
    else if (value->data_size == 0)
    {
        // In this case we need to allocate new storage space
        // where previously none was allocated. This means
        // that a full allocation is required.
        value->data.extended.as_ptr = BoltMem_allocate(data_size);
        value->data_size = data_size;
    }
    else if (data_size == 0)
    {
        // In this case, we are moving from previously having
        // data to no longer requiring any storage space. This
        // means that we can free up the previously-allocated
        // space.
        value->data.extended.as_ptr = BoltMem_deallocate(value->data.extended.as_ptr, value->data_size);
        value->data_size = 0;
    }
    else
    {
        // Finally, this case deals with previous allocation
        // and a new allocation requirement, but of different
        // sizes. Here, we `realloc`, which should be more
        // efficient than a naïve deallocation followed by a
        // brand new allocation.
        value->data.extended.as_ptr = BoltMem_reallocate(value->data.extended.as_ptr, value->data_size, data_size);
        value->data_size = data_size;
    }
}

void _BoltValue_copyData(struct BoltValue* value, const void* data, size_t offset, size_t length)
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
void _BoltValue_recycle(struct BoltValue* value)
{
    enum BoltType type = BoltValue_type(value);
    if (type == BOLT_LIST)
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
    else if (type == BOLT_STRUCTURE || type == BOLT_REQUEST || type == BOLT_SUMMARY)
    {
        for (long i = 0; i < value->size; i++)
        {
            BoltValue_toNull(&value->data.extended.as_value[i]);
        }
    }
}

void _BoltValue_setType(struct BoltValue* value, enum BoltType type, char is_array, int size)
{
    assert(type < 0x80);
    value->type = (char)(type);
    value->is_array = is_array;
    value->size = size;
}

void _BoltValue_to(struct BoltValue* value, enum BoltType type, char is_array, int size, const void* data, size_t data_size)
{
    _BoltValue_recycle(value);
    _BoltValue_allocate(value, data_size);
    if (data != NULL)
    {
        _BoltValue_copyData(value, data, 0, data_size);
    }
    _BoltValue_setType(value, type, is_array, size);
}


/**
 * Resize a value that contains multiple sub-values.
 *
 * @param value
 * @param size
 * @param multiplier
 */
void _BoltValue_resize(struct BoltValue* value, int32_t size, int multiplier)
{
    if (size > value->size)
    {
        // grow physically
        size_t unit_size = sizeof(struct BoltValue);
        size_t new_data_size = multiplier * unit_size * size;
        size_t old_data_size = value->data_size;
        _BoltValue_allocate(value, new_data_size);
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
        _BoltValue_allocate(value, new_data_size);
    }
    else
    {
        // same size - do nothing
    }
}
