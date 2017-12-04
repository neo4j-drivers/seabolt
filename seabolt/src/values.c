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

#include "mem.h"
#include "values.h"

#define sizeof_n(type, n) (size_t)((n) >= 0 ? sizeof(type) * (n) : 0)

#define to_bit(x) (char)((x) == 0 ? 0 : 1);


static const struct BoltValue BOLT_NULL_VALUE = {BOLT_NULL, 0, 0, 0, 0, NULL};


/**
 * Allocate, reallocate or free memory for data storage.
 *
 * Since we recycle values, we can also potentially recycle
 * the dynamically-allocated storage.
 *
 * @param value the value in which to allocate storage
 * @param physical_size the number of bytes of storage required
 */
void _BoltValue_allocate(struct BoltValue* value, size_t physical_size)
{
    if (physical_size == value->physical_size)
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
    else if (value->physical_size == 0)
    {
        // In this case we need to allocate new storage space
        // where previously none was allocated. This means
        // that a full allocation is required.
        assert(value->data.as_ptr == NULL);
        value->data.as_ptr = BoltMem_allocate(physical_size);
        value->physical_size = physical_size;
    }
    else if (physical_size == 0)
    {
        // In this case, we are moving from previously having
        // data to no longer requiring any storage space. This
        // means that we can free up the previously-allocated
        // space.
        assert(value->data.as_ptr != NULL);
        value->data.as_ptr = BoltMem_deallocate(value->data.as_ptr, value->physical_size);
        value->physical_size = 0;
    }
    else
    {
        // Finally, this case deals with previous allocation
        // and a new allocation requirement, but of different
        // sizes. Here, we `realloc`, which should be more
        // efficient than a naïve deallocation followed by a
        // brand new allocation.
        assert(value->data.as_ptr != NULL);
        value->data.as_ptr = BoltMem_reallocate(value->data.as_ptr, value->physical_size, physical_size);
        value->physical_size = physical_size;
    }
}

void _BoltValue_copyData(struct BoltValue* value, const void* data, size_t offset, size_t length)
{
    if (length > 0)
    {
        memcpy(value->data.as_char + offset, data, length);
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
    if (value->type == BOLT_LIST)
    {
        for (long i = 0; i < value->logical_size; i++)
        {
            BoltValue_toNull(&value->data.as_value[i]);
        }
    }
    else if (value->type == BOLT_UTF8 && value->is_array)
    {
        for (long i = 0; i < value->logical_size; i++)
        {
            struct array_t string = value->data.as_array[i];
            if (string.size > 0)
            {
                BoltMem_deallocate(string.data.as_ptr, (size_t)(string.size));
            }
        }
    }
    else if (value->type == BOLT_UTF8_DICTIONARY)
    {
        for (long i = 0; i < 2 * value->logical_size; i++)
        {
            BoltValue_toNull(&value->data.as_value[i]);
        }
    }
    else if (value->type == BOLT_UTF16_DICTIONARY)
    {
        // TODO
    }
    else if (value->type == BOLT_STRUCTURE)
    {
        for (long i = 0; i < value->logical_size; i++)
        {
            BoltValue_toNull(&value->data.as_value[i]);
        }
    }
}

void _BoltValue_to(struct BoltValue* value, enum BoltType type, int code, int is_array,
                   const void* data, int logical_size, size_t physical_size)
{
    _BoltValue_recycle(value);
    _BoltValue_allocate(value, physical_size);
    if (data != NULL)
    {
        _BoltValue_copyData(value, data, 0, physical_size);
    }
    value->type = type;
    value->code = code;
    value->is_array = is_array;
    value->logical_size = logical_size;
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
    if (size > value->logical_size)
    {
        // grow physically
        size_t unit_size = sizeof(struct BoltValue);
        size_t new_physical_size = multiplier * unit_size * size;
        size_t old_physical_size = value->physical_size;
        _BoltValue_allocate(value, new_physical_size);
        // grow logically
        for (size_t offset = old_physical_size; offset < new_physical_size; offset += unit_size)
        {
            _BoltValue_copyData(value, &BOLT_NULL_VALUE, offset, unit_size);
        }
        value->logical_size = size;
    }
    else if (size < value->logical_size)
    {
        // shrink logically
        for (long i = multiplier * size; i < multiplier * value->logical_size; i++)
        {
            BoltValue_toNull(&value->data.as_value[i]);
        }
        value->logical_size = size;
        // shrink physically
        size_t new_physical_size = multiplier * sizeof_n(struct BoltValue, size);
        _BoltValue_allocate(value, new_physical_size);
    }
    else
    {
        // same size - do nothing
    }
}


struct BoltValue* BoltValue_create()
{
    size_t size = sizeof(struct BoltValue);
    struct BoltValue* value = BoltMem_allocate(size);
    value->type = BOLT_NULL;
    value->code = 0;
    value->is_array = 0;
    value->logical_size = 0;
    value->physical_size = 0;
    value->data.as_ptr = NULL;
    return value;
}

void BoltValue_destroy(struct BoltValue* value)
{
    BoltValue_toNull(value);
    BoltMem_deallocate(value, sizeof(struct BoltValue));
}


/*********************************************************************
 * Null
 */

void BoltValue_toNull(struct BoltValue* value)
{
    _BoltValue_to(value, BOLT_NULL, 0, 0, NULL, 0, 0);
}


/*********************************************************************
 * List
 */

void BoltValue_toList(struct BoltValue* value, int32_t size)
{
    size_t unit_size = sizeof(struct BoltValue);
    size_t physical_size = unit_size * size;
    _BoltValue_recycle(value);
    _BoltValue_allocate(value, physical_size);
    for (size_t offset = 0; offset < physical_size; offset += unit_size)
    {
        _BoltValue_copyData(value, &BOLT_NULL_VALUE, offset, unit_size);
    }
    value->type = BOLT_LIST;
    value->code = 0;
    value->is_array = 0;
    value->logical_size = size;
}

void BoltList_resize(struct BoltValue* value, int32_t size)
{
    assert(value->type == BOLT_LIST);
    _BoltValue_resize(value, size, 1);
}

struct BoltValue* BoltList_at(const struct BoltValue* value, int32_t index)
{
    assert(value->type == BOLT_LIST);
    return &value->data.as_value[index];
}


/*********************************************************************
 * Bit / BitArray
 */

void BoltValue_toBit(struct BoltValue* value, char x)
{
    _BoltValue_to(value, BOLT_BIT, 0, 0, &x, 1, sizeof(char));
}

char BoltBit_get(const struct BoltValue* value)
{
    return to_bit(value->data.as_char[0]);
}

void BoltValue_toBitArray(struct BoltValue* value, char* array, int32_t size)
{
    _BoltValue_to(value, BOLT_BIT, 0, 1, array, size, sizeof_n(char, size));
}

char BoltBitArray_get(const struct BoltValue* value, int32_t index)
{
    return to_bit(value->data.as_char[index]);
}


/*********************************************************************
 * Byte / ByteArray
 */

void BoltValue_toByte(struct BoltValue* value, char x)
{
    _BoltValue_to(value, BOLT_BYTE, 0, 0, &x, 1, sizeof(char));
}

char BoltByte_get(const struct BoltValue* value)
{
    return value->data.as_char[0];
}

void BoltValue_toByteArray(struct BoltValue* value, char* array, int32_t size)
{
    _BoltValue_to(value, BOLT_BYTE, 0, 1, array, size, sizeof_n(char, size));
}

char BoltByteArray_get(const struct BoltValue* value, int32_t index)
{
    return value->data.as_char[index];
}


/*********************************************************************
 * Char16 / Char16Array
 */

void BoltValue_toChar16(struct BoltValue* value, uint16_t x)
{
    _BoltValue_to(value, BOLT_CHAR16, 0, 0, &x, 1, sizeof(uint16_t));
}

void BoltValue_toChar16Array(struct BoltValue* value, uint16_t* array, int32_t size)
{
    _BoltValue_to(value, BOLT_CHAR16, 0, 1, array, size, sizeof_n(uint16_t, size));
}


/*********************************************************************
 * Char32 / Char32Array
 */

void BoltValue_toChar32(struct BoltValue* value, uint32_t x)
{
    _BoltValue_to(value, BOLT_CHAR32, 0, 0, &x, 1, sizeof(uint32_t));
}

void BoltValue_toChar32Array(struct BoltValue* value, uint32_t* array, int32_t size)
{
    _BoltValue_to(value, BOLT_CHAR32, 0, 1, array, size, sizeof_n(uint32_t, size));
}


/*********************************************************************
 * UTF8 / UTF8Array
 */

void BoltValue_toUTF8(struct BoltValue* value, char* string, int32_t size)
{
    _BoltValue_to(value, BOLT_UTF8, 0, 0, string, size, sizeof_n(char, size));
}

char* BoltUTF8_get(struct BoltValue* value)
{
    return value->data.as_char;
}

void BoltValue_toUTF8Array(struct BoltValue* value, int32_t size)
{
    _BoltValue_to(value, BOLT_UTF8, 0, 1, NULL, size, sizeof_n(struct array_t, size));
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


/*********************************************************************
 * UTF8Dictionary
 */

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
    value->code = 0;
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


/*********************************************************************
 * UTF16 / UTF16Array
 */

void BoltValue_toUTF16(struct BoltValue* value, uint16_t* string, int32_t size)
{
    _BoltValue_to(value, BOLT_UTF16, 0, 0, string, size, sizeof_n(uint16_t, size));
}


/*********************************************************************
 * Num8 / Num8Array
 */

void BoltValue_toNum8(struct BoltValue* value, uint8_t x)
{
    _BoltValue_to(value, BOLT_NUM8, 0, 0, &x, 1, sizeof(uint8_t));
}

uint8_t BoltNum8_get(const struct BoltValue* value)
{
    return value->data.as_uint8[0];
}

void BoltValue_toNum8Array(struct BoltValue* value, uint8_t* array, int32_t size)
{
    _BoltValue_to(value, BOLT_NUM8, 0, 1, array, size, sizeof_n(uint8_t, size));
}

uint8_t BoltNum8Array_get(const struct BoltValue* value, int32_t index)
{
    return value->data.as_uint8[index];
}


/*********************************************************************
 * Num16 / Num16Array
 */

void BoltValue_toNum16(struct BoltValue* value, uint16_t x)
{
    _BoltValue_to(value, BOLT_NUM16, 0, 0, &x, 1, sizeof(uint16_t));
}

uint16_t BoltNum16_get(const struct BoltValue* value)
{
    return value->data.as_uint16[0];
}

void BoltValue_toNum16Array(struct BoltValue* value, uint16_t* array, int32_t size)
{
    _BoltValue_to(value, BOLT_NUM16, 0, 1, array, size, sizeof_n(uint16_t, size));
}

uint16_t BoltNum16Array_get(const struct BoltValue* value, int32_t index)
{
    return value->data.as_uint16[index];
}


/*********************************************************************
 * Num32 / Num32Array
 */

void BoltValue_toNum32(struct BoltValue* value, uint32_t x)
{
    _BoltValue_to(value, BOLT_NUM32, 0, 0, &x, 1, sizeof(uint32_t));
}

uint32_t BoltNum32_get(const struct BoltValue* value)
{
    return value->data.as_uint32[0];
}

void BoltValue_toNum32Array(struct BoltValue* value, uint32_t* array, int32_t size)
{
    _BoltValue_to(value, BOLT_NUM32, 0, 1, array, size, sizeof_n(uint32_t, size));
}

uint32_t BoltNum32Array_get(const struct BoltValue* value, int32_t index)
{
    return value->data.as_uint32[index];
}


/*********************************************************************
 * Num64 / Num64Array
 */

void BoltValue_toNum64(struct BoltValue* value, uint64_t x)
{
    _BoltValue_to(value, BOLT_NUM64, 0, 0, &x, 1, sizeof(uint64_t));
}

uint64_t BoltNum64_get(const struct BoltValue* value)
{
    return value->data.as_uint64[0];
}

void BoltValue_toNum64Array(struct BoltValue* value, uint64_t* array, int32_t size)
{
    _BoltValue_to(value, BOLT_NUM64, 0, 1, array, size, sizeof_n(uint64_t, size));
}

uint64_t BoltNum64Array_get(const struct BoltValue* value, int32_t index)
{
    return value->data.as_uint64[index];
}


/*********************************************************************
 * Int8 / Int8Array
 */

void BoltValue_toInt8(struct BoltValue* value, int8_t x)
{
    _BoltValue_to(value, BOLT_INT8, 0, 0, &x, 1, sizeof(int8_t));
}

int8_t BoltInt8_get(const struct BoltValue* value)
{
    return value->data.as_int8[0];
}

void BoltValue_toInt8Array(struct BoltValue* value, int8_t* array, int32_t size)
{
    _BoltValue_to(value, BOLT_INT8, 0, 1, array, size, sizeof_n(int8_t, size));
}

int8_t BoltInt8Array_get(const struct BoltValue* value, int32_t index)
{
    return value->data.as_int8[index];
}


/*********************************************************************
 * Int16 / Int16Array
 */

void BoltValue_toInt16(struct BoltValue* value, int16_t x)
{
    _BoltValue_to(value, BOLT_INT16, 0, 0, &x, 1, sizeof(int16_t));
}

int16_t BoltInt16_get(const struct BoltValue* value)
{
    return value->data.as_int16[0];
}

void BoltValue_toInt16Array(struct BoltValue* value, int16_t* array, int32_t size)
{
    _BoltValue_to(value, BOLT_INT16, 0, 1, array, size, sizeof_n(int16_t, size));
}

int16_t BoltInt16Array_get(const struct BoltValue* value, int32_t index)
{
    return value->data.as_int16[index];
}


/*********************************************************************
 * Int32 / Int32Array
 */

void BoltValue_toInt32(struct BoltValue* value, int32_t x)
{
    _BoltValue_to(value, BOLT_INT32, 0, 0, &x, 1, sizeof(int32_t));
}

int32_t BoltInt32_get(const struct BoltValue* value)
{
    return value->data.as_int32[0];
}

void BoltValue_toInt32Array(struct BoltValue* value, int32_t* array, int32_t size)
{
    _BoltValue_to(value, BOLT_INT32, 0, 1, array, size, sizeof_n(int32_t, size));
}

int32_t BoltInt32Array_get(const struct BoltValue* value, int32_t index)
{
    return value->data.as_int32[index];
}


/*********************************************************************
 * Int64 / Int64Array
 */

void BoltValue_toInt64(struct BoltValue* value, int64_t x)
{
    _BoltValue_to(value, BOLT_INT64, 0, 0, &x, 1, sizeof(int64_t));
}

int64_t BoltInt64_get(const struct BoltValue* value)
{
    return value->data.as_int64[0];
}

void BoltValue_toInt64Array(struct BoltValue* value, int64_t* array, int32_t size)
{
    _BoltValue_to(value, BOLT_INT64, 0, 1, array, size, sizeof_n(int64_t, size));
}

int64_t BoltInt64Array_get(const struct BoltValue* value, int32_t index)
{
    return value->data.as_int64[index];
}


/*********************************************************************
 * Float32 / Float32Array
 */

void BoltValue_toFloat32(struct BoltValue* value, float x)
{
    _BoltValue_to(value, BOLT_FLOAT32, 0, 0, &x, 1, sizeof(float));
}

float BoltFloat32_get(const struct BoltValue* value)
{
    return value->data.as_float[0];
}

void BoltValue_toFloat32Array(struct BoltValue* value, float* array, int32_t size)
{
    _BoltValue_to(value, BOLT_FLOAT32, 0, 1, array, size, sizeof_n(float, size));
}

float BoltFloat32Array_get(const struct BoltValue* value, int32_t index)
{
    return value->data.as_float[index];
}


/*********************************************************************
 * Float32Pair / Float32PairArray
 */


//void bolt_put_float32_pair(struct BoltValue* value, float x, float y);
//void bolt_put_float32_pair_array(struct BoltValue* value, struct float_pair* array, int32_t size);



/*********************************************************************
 * Float32Triple / Float32TripleArray
 */

//void bolt_put_float32_triple(struct BoltValue* value, float x, float y, float z);
//void bolt_put_float32_triple_array(struct BoltValue* value, struct float_triple* array, int32_t size);



/*********************************************************************
 * Float32Quad / Float32QuadArray
 */

//void bolt_put_float32_quad(struct BoltValue* value, float x, float y, float z, float a);
//void bolt_put_float32_quad_array(struct BoltValue* value, struct float_quad* array, int32_t size);



/*********************************************************************
 * Float64 / Float64Array
 */

void bolt_put_float64(struct BoltValue* value, double x)
{
    _BoltValue_to(value, BOLT_FLOAT64, 0, 0, &x, 1, sizeof(double));
}

double bolt_get_float64(struct BoltValue* value)
{
    return value->data.as_double[0];
}

//void bolt_put_float64_array(struct BoltValue* value, double* array, int32_t size);



/*********************************************************************
 * Float64Pair / Float64PairArray
 */

//void bolt_put_float64_pair(struct BoltValue* value, double x, double y);
//void bolt_put_float64_pair_array(struct BoltValue* value, struct double_pair* array, int32_t size);



/*********************************************************************
 * Float64Triple / Float64TripleArray
 */

//void bolt_put_float64_triple(struct BoltValue* value, double x, double y, double z);
//void bolt_put_float64_triple_array(struct BoltValue* value, struct double_triple* array, int32_t size);



/*********************************************************************
 * Float64Quad / Float64QuadArray
 */

//void bolt_put_float64_quad(struct BoltValue* value, double x, double y, double z, double a);
//void bolt_put_float64_quad_array(struct BoltValue* value, struct double_quad* array, int32_t size);



/*********************************************************************
 * Structure / StructureArray
 */

void BoltValue_toStructure(struct BoltValue* value, int16_t code, int32_t size)
{
    size_t unit_size = sizeof(struct BoltValue);
    size_t physical_size = unit_size * size;
    _BoltValue_recycle(value);
    _BoltValue_allocate(value, physical_size);
    for (size_t offset = 0; offset < physical_size; offset += unit_size)
    {
        _BoltValue_copyData(value, &BOLT_NULL_VALUE, offset, unit_size);
    }
    value->type = BOLT_STRUCTURE;
    value->code = code;
    value->is_array = 0;
    value->logical_size = size;
}

struct BoltValue* BoltStructure_at(const struct BoltValue* value, int32_t index)
{
    assert(value->type == BOLT_STRUCTURE);
    return &value->data.as_value[index];
}
