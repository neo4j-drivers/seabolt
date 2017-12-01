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

#ifndef SEABOLT_VALUES
#define SEABOLT_VALUES

#include <limits.h>
#include <malloc.h>
#include <memory.h>
#include <assert.h>

#if CHAR_BIT != 8
#error "Cannot compile if `char` is not 8-bit"
#endif

#define sizeof_n(type, n) (size_t)((n) >= 0 ? sizeof(type) * (n) : 0)

#define to_bit(x) (char)((x) == 0 ? 0 : 1);


static const size_t SIZE_OF_SIZE = sizeof(int32_t);


enum BoltType
{
    BOLT_NULL,                          /* ALSO IN BOLT v1 (as Null) */
    BOLT_LIST,                          /* ALSO IN BOLT v1 (as List) */
    BOLT_BIT,                           /* ALSO IN BOLT v1 (as Boolean) */
    BOLT_BYTE,
    BOLT_CHAR16,
    BOLT_CHAR32,
    BOLT_UTF8,                          /* ALSO IN BOLT v1 (as String) */
    BOLT_UTF8_DICTIONARY,               /* ALSO IN BOLT v1 (as Map) */
    BOLT_UTF16,
    BOLT_UTF16_DICTIONARY,
    BOLT_NUM8,
    BOLT_NUM16,
    BOLT_NUM32,
    BOLT_NUM64,
    BOLT_INT8,
    BOLT_INT16,
    BOLT_INT32,
    BOLT_INT64,                         /* ALSO IN BOLT v1 (as Integer) */
    BOLT_FLOAT32,
    BOLT_FLOAT32_PAIR,
    BOLT_FLOAT32_TRIPLE,
    BOLT_FLOAT32_QUAD,
    BOLT_FLOAT64,                       /* ALSO IN BOLT v1 (as Float) */
    BOLT_FLOAT64_PAIR,
    BOLT_FLOAT64_TRIPLE,
    BOLT_FLOAT64_QUAD,
    BOLT_STRUCTURE,                     /* ALSO IN BOLT v1 (as Structure) */
    BOLT_REQUEST,  // requests match 10xxxxxx
    BOLT_SUMMARY,  // summaries match 11xxxxxx
};

struct BoltValue
{
    enum BoltType type;
    int code;
    int is_array;
    long logical_size;
    size_t physical_size;
    union
    {
        void* as_ptr;
        char* as_char;
        uint8_t* as_uint8;
        uint16_t* as_uint16;
        uint32_t* as_uint32;
        uint64_t* as_uint64;
        int8_t* as_int8;
        int16_t* as_int16;
        int32_t* as_int32;
        int64_t* as_int64;
        float* as_float;
        double* as_double;
        struct BoltValue* as_value;
    } data;
};


struct float_pair
{
    float x;
    float y;
};

struct float_triple
{
    float x;
    float y;
    float z;
};

struct float_quad
{
    float x;
    float y;
    float z;
    float a;
};

struct double_pair
{
    double x;
    double y;
};

struct double_triple
{
    double x;
    double y;
    double z;
};

struct double_quad
{
    double x;
    double y;
    double z;
    double a;
};


static size_t __bolt_value_memory = 0;


void bolt_null_list(struct BoltValue* value);

void bolt_null_utf8_dictionary(struct BoltValue* value);


/**
 * Allocate, reallocate or free memory for data storage.
 *
 * Since we recycle values, we can also potentially recycle
 * the dynamically-allocated storage.
 *
 * @param value the value in which to allocate storage
 * @param physical_size the number of bytes of storage required
 */
void _bolt_alloc(struct BoltValue* value, size_t physical_size)
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
        // that a full `malloc` is required.
        assert(value->data.as_ptr == NULL);
        value->data.as_ptr = malloc(physical_size);
        __bolt_value_memory += physical_size;
        fprintf(stderr, "\x1B[36mAllocated %ld bytes (balance: %lu)\x1B[0m\n",
                physical_size, __bolt_value_memory);
        value->physical_size = physical_size;
    }
    else if (physical_size == 0)
    {
        // In this case, we are moving from previously having
        // data to no longer requiring any storage space. This
        // means that we can free up the previously-allocated
        // space.
        assert(value->data.as_ptr != NULL);
        free(value->data.as_ptr);
        value->data.as_ptr = NULL;
        __bolt_value_memory -= value->physical_size;
        fprintf(stderr, "\x1B[36mFreed %ld bytes (balance: %lu)\x1B[0m\n",
                value->physical_size, __bolt_value_memory);
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
        value->data.as_ptr = realloc(value->data.as_ptr, physical_size);
        __bolt_value_memory = __bolt_value_memory - value->physical_size + physical_size;
        fprintf(stderr, "\x1B[36mReallocated %ld bytes as %ld bytes (balance: %lu)\x1B[0m\n",
                value->physical_size, physical_size, __bolt_value_memory);
        value->physical_size = physical_size;
    }
}

void _bolt_copy_data(struct BoltValue* value, const void* data, size_t offset, size_t length)
{
    if (length > 0)
    {
        memcpy(value->data.as_char + offset, data, length);
    }
}

/**
 * Set any nested values to null.
 *
 * @param value
 */
void _bolt_null_items(struct BoltValue* value)
{
    if (value->type == BOLT_LIST)
    {
        bolt_null_list(value);
    }
    else if (value->type == BOLT_UTF8_DICTIONARY)
    {
        bolt_null_utf8_dictionary(value);
    }
}

void _bolt_put(struct BoltValue* value, enum BoltType type, int code, int is_array,
               const void* data, int logical_size, size_t physical_size)
{
    _bolt_null_items(value);
    _bolt_alloc(value, physical_size);
    _bolt_copy_data(value, data, 0, physical_size);
    value->type = type;
    value->code = code;
    value->is_array = is_array;
    value->logical_size = logical_size;
}


struct BoltValue bolt_value()
{
    struct BoltValue value;
    value.type = BOLT_NULL;
    value.code = 0;
    value.is_array = 0;
    value.logical_size = 0;
    value.physical_size = 0;
    value.data.as_ptr = NULL;
    return value;
}


static const struct BoltValue BOLT_NULL_VALUE = {BOLT_NULL, 0, 0, 0, 0, NULL};


/*********************************************************************
 * Null
 */

void bolt_null(struct BoltValue* value)
{
    _bolt_put(value, BOLT_NULL, 0, 0, NULL, 0, 0);
}


/*********************************************************************
 * List
 */

void bolt_put_list(struct BoltValue* value, int32_t size)
{
    size_t unit_size = sizeof(struct BoltValue);
    size_t physical_size = unit_size * size;
    _bolt_null_items(value);
    _bolt_alloc(value, physical_size);
    for (size_t offset = 0; offset < physical_size; offset += unit_size)
    {
        _bolt_copy_data(value, &BOLT_NULL_VALUE, offset, unit_size);
    }
    value->type = BOLT_LIST;
    value->code = 0;
    value->is_array = 0;
    value->logical_size = size;
}

void bolt_null_list(struct BoltValue* value)
{
    assert(value->type == BOLT_LIST);
    for (long i = 0; i < value->logical_size; i++)
    {
        bolt_null(&value->data.as_value[i]);
    }
}

void bolt_resize_list(struct BoltValue* value, int32_t size)
{
    assert(value->type == BOLT_LIST);
    if (size > value->logical_size)
    {
        // grow physically
        size_t unit_size = sizeof(struct BoltValue);
        size_t new_physical_size = unit_size * size;
        size_t old_physical_size = value->physical_size;
        _bolt_alloc(value, new_physical_size);
        // grow logically
        for (size_t offset = old_physical_size; offset < new_physical_size; offset += unit_size)
        {
            _bolt_copy_data(value, &BOLT_NULL_VALUE, offset, unit_size);
        }
        value->logical_size = size;
    }
    else if (size < value->logical_size)
    {
        // shrink logically
        for (long i = size; i < value->logical_size; i++)
        {
            bolt_null(&value->data.as_value[i]);
        }
        value->logical_size = size;
        // shrink physically
        size_t new_physical_size = sizeof_n(struct BoltValue, size);
        _bolt_alloc(value, new_physical_size);
    }
    else
    {
        // same size - do nothing
    }
}

struct BoltValue* bolt_get_list_at(const struct BoltValue* value, int32_t index)
{
    assert(value->type == BOLT_LIST);
    return &value->data.as_value[index];
}


/*********************************************************************
 * Bit / BitArray
 */

void bolt_put_bit(struct BoltValue* value, char x)
{
    _bolt_put(value, BOLT_BIT, 0, 0, &x, 1, sizeof(char));
}

char bolt_get_bit(const struct BoltValue* value)
{
    return to_bit(value->data.as_char[0]);
}

void bolt_put_bit_array(struct BoltValue* value, char* array, int32_t size)
{
    _bolt_put(value, BOLT_BIT, 0, 1, array, size, sizeof_n(char, size));
}

char bolt_get_bit_array_at(const struct BoltValue* value, int32_t index)
{
    return to_bit(value->data.as_char[index]);
}


/*********************************************************************
 * Byte / ByteArray
 */

void bolt_put_byte(struct BoltValue* value, char x)
{
    _bolt_put(value, BOLT_BYTE, 0, 0, &x, 1, sizeof(char));
}

char bolt_get_byte(const struct BoltValue* value)
{
    return value->data.as_char[0];
}

void bolt_put_byte_array(struct BoltValue* value, char* array, int32_t size)
{
    _bolt_put(value, BOLT_BYTE, 0, 1, array, size, sizeof_n(char, size));
}

char bolt_get_byte_array_at(const struct BoltValue* value, int32_t index)
{
    return value->data.as_char[index];
}


/*********************************************************************
 * Char16 / Char16Array
 */

void bolt_put_char16(struct BoltValue* value, uint16_t x)
{
    _bolt_put(value, BOLT_CHAR16, 0, 0, &x, 1, sizeof(uint16_t));
}

void bolt_put_char16_array(struct BoltValue* value, uint16_t* array, int32_t size)
{
    _bolt_put(value, BOLT_CHAR16, 0, 1, array, size, sizeof_n(uint16_t, size));
}


/*********************************************************************
 * Char32 / Char32Array
 */

void bolt_put_char32(struct BoltValue* value, uint32_t x)
{
    _bolt_put(value, BOLT_CHAR32, 0, 0, &x, 1, sizeof(uint32_t));
}

void bolt_put_char32_array(struct BoltValue* value, uint32_t* array, int32_t size)
{
    _bolt_put(value, BOLT_CHAR32, 0, 1, array, size, sizeof_n(uint32_t, size));
}


/*********************************************************************
 * UTF8 / UTF8Array
 */

void bolt_put_utf8(struct BoltValue* value, char* string, int32_t size)
{
    _bolt_put(value, BOLT_UTF8, 0, 0, string, size, sizeof_n(char, size));
}

char* bolt_get_utf8(struct BoltValue* value)
{
    return value->data.as_char;
}

void bolt_put_utf8_array(struct BoltValue* value)
{
    _bolt_put(value, BOLT_UTF8, 0, 1, NULL, 0, 0);
}

void bolt_put_utf8_array_next(struct BoltValue* value, char* string, int32_t size)
{
    assert(value->type == BOLT_UTF8);
    assert(value->is_array);
    value->logical_size += 1;
    size_t old_physical_size = value->physical_size;
    _bolt_alloc(value, old_physical_size + SIZE_OF_SIZE + size);
    _bolt_copy_data(value, &size, old_physical_size, SIZE_OF_SIZE);
    _bolt_copy_data(value, string, old_physical_size + SIZE_OF_SIZE, (size_t)(size));
}

int32_t bolt_get_utf8_array_size_at(struct BoltValue* value, int32_t index)
{
    size_t offset = 0;
    int32_t size = 0;
    memcpy(&size, &value->data.as_char[offset], SIZE_OF_SIZE);
    for (int i = 0; i < index; i++)
    {
        offset += SIZE_OF_SIZE + size;
        memcpy(&size, &value->data.as_char[offset], SIZE_OF_SIZE);
    }
    return size;
}

char* bolt_get_utf8_array_at(struct BoltValue* value, int32_t index)
{
    size_t offset = 0;
    int32_t size = 0;
    memcpy(&size, &value->data.as_char[offset], SIZE_OF_SIZE);
    for (int i = 0; i < index; i++)
    {
        offset += SIZE_OF_SIZE + size;
        memcpy(&size, &value->data.as_char[offset], SIZE_OF_SIZE);
    }
    return &value->data.as_char[offset + SIZE_OF_SIZE];
}


/*********************************************************************
 * UTF8Dictionary
 */

void bolt_put_utf8_dictionary(struct BoltValue* value, int32_t size)
{
    size_t unit_size = sizeof(struct BoltValue);
    size_t physical_size = 2 * unit_size * size;
    _bolt_null_items(value);
    _bolt_alloc(value, physical_size);
    for (size_t offset = 0; offset < physical_size; offset += unit_size)
    {
        _bolt_copy_data(value, &BOLT_NULL_VALUE, offset, unit_size);
    }
    value->type = BOLT_UTF8_DICTIONARY;
    value->code = 0;
    value->is_array = 0;
    value->logical_size = size;
}

void bolt_null_utf8_dictionary(struct BoltValue* value)
{
    assert(value->type == BOLT_UTF8_DICTIONARY);
    for (long i = 0; i < 2 * value->logical_size; i++)
    {
        bolt_null(&value->data.as_value[i]);
    }
}

void bolt_resize_utf8_dictionary(struct BoltValue* value, int32_t size)
{
    assert(value->type == BOLT_UTF8_DICTIONARY);
    if (size > value->logical_size)
    {
        // grow physically
        size_t unit_size = sizeof(struct BoltValue);
        size_t new_physical_size = 2 * unit_size * size;
        size_t old_physical_size = value->physical_size;
        _bolt_alloc(value, new_physical_size);
        // grow logically
        for (size_t offset = old_physical_size; offset < new_physical_size; offset += unit_size)
        {
            _bolt_copy_data(value, &BOLT_NULL_VALUE, offset, unit_size);
        }
        value->logical_size = size;
    }
    else if (size < value->logical_size)
    {
        // shrink logically
        for (long i = 2 * size; i < 2 * value->logical_size; i++)
        {
            bolt_null(&value->data.as_value[i]);
        }
        value->logical_size = size;
        // shrink physically
        size_t new_physical_size = 2 * sizeof_n(struct BoltValue, size);
        _bolt_alloc(value, new_physical_size);
    }
    else
    {
        // same size - do nothing
    }
}

void bolt_put_utf8_dictionary_key_at(struct BoltValue* value, int32_t index, char* key, int32_t key_size)
{
    assert(value->type == BOLT_UTF8_DICTIONARY);
    bolt_put_utf8(&value->data.as_value[2 * index], key, key_size);
}

struct BoltValue* bolt_get_utf8_dictionary_key_at(const struct BoltValue* value, int32_t index)
{
    assert(value->type == BOLT_UTF8_DICTIONARY);
    struct BoltValue* key = &value->data.as_value[2 * index];
    return key->type == BOLT_UTF8 ? key : NULL;
}

struct BoltValue* bolt_get_utf8_dictionary_at(const struct BoltValue* value, int32_t index)
{
    assert(value->type == BOLT_UTF8_DICTIONARY);
    return &value->data.as_value[2 * index + 1];
}


/*********************************************************************
 * UTF16 / UTF16Array
 */

void bolt_put_utf16(struct BoltValue* value, uint16_t* string, int32_t size)
{
    _bolt_put(value, BOLT_UTF16, 0, 0, string, size, sizeof_n(uint16_t, size));
}


/*********************************************************************
 * Num8 / Num8Array
 */

void bolt_put_num8(struct BoltValue* value, uint8_t x)
{
    _bolt_put(value, BOLT_NUM8, 0, 0, &x, 1, sizeof(uint8_t));
}

uint8_t bolt_get_num8(const struct BoltValue* value)
{
    return value->data.as_uint8[0];
}

void bolt_put_num8_array(struct BoltValue* value, uint8_t* array, int32_t size)
{
    _bolt_put(value, BOLT_NUM8, 0, 1, array, size, sizeof_n(uint8_t, size));
}

uint8_t bolt_get_num8_array_at(const struct BoltValue* value, int32_t index)
{
    return value->data.as_uint8[index];
}


/*********************************************************************
 * Num16 / Num16Array
 */

void bolt_put_num16(struct BoltValue* value, uint16_t x)
{
    _bolt_put(value, BOLT_NUM16, 0, 0, &x, 1, sizeof(uint16_t));
}

uint16_t bolt_get_num16(const struct BoltValue* value)
{
    return value->data.as_uint16[0];
}

void bolt_put_num16_array(struct BoltValue* value, uint16_t* array, int32_t size)
{
    _bolt_put(value, BOLT_NUM16, 0, 1, array, size, sizeof_n(uint16_t, size));
}

uint16_t bolt_get_num16_array_at(const struct BoltValue* value, int32_t index)
{
    return value->data.as_uint16[index];
}


/*********************************************************************
 * Num32 / Num32Array
 */

void bolt_put_num32(struct BoltValue* value, uint32_t x)
{
    _bolt_put(value, BOLT_NUM32, 0, 0, &x, 1, sizeof(uint32_t));
}

uint32_t bolt_get_num32(const struct BoltValue* value)
{
    return value->data.as_uint32[0];
}

void bolt_put_num32_array(struct BoltValue* value, uint32_t* array, int32_t size)
{
    _bolt_put(value, BOLT_NUM32, 0, 1, array, size, sizeof_n(uint32_t, size));
}

uint32_t bolt_get_num32_array_at(const struct BoltValue* value, int32_t index)
{
    return value->data.as_uint32[index];
}


/*********************************************************************
 * Num64 / Num64Array
 */

void bolt_put_num64(struct BoltValue* value, uint64_t x)
{
    _bolt_put(value, BOLT_NUM64, 0, 0, &x, 1, sizeof(uint64_t));
}

uint64_t bolt_get_num64(const struct BoltValue* value)
{
    return value->data.as_uint64[0];
}

void bolt_put_num64_array(struct BoltValue* value, uint64_t* array, int32_t size)
{
    _bolt_put(value, BOLT_NUM64, 0, 1, array, size, sizeof_n(uint64_t, size));
}

uint64_t bolt_get_num64_array_at(const struct BoltValue* value, int32_t index)
{
    return value->data.as_uint64[index];
}


/*********************************************************************
 * Int8 / Int8Array
 */

void bolt_put_int8(struct BoltValue* value, int8_t x)
{
    _bolt_put(value, BOLT_INT8, 0, 0, &x, 1, sizeof(int8_t));
}

int8_t bolt_get_int8(const struct BoltValue* value)
{
    return value->data.as_int8[0];
}

void bolt_put_int8_array(struct BoltValue* value, int8_t* array, int32_t size)
{
    _bolt_put(value, BOLT_INT8, 0, 1, array, size, sizeof_n(int8_t, size));
}

int8_t bolt_get_int8_array_at(const struct BoltValue* value, int32_t index)
{
    return value->data.as_int8[index];
}


/*********************************************************************
 * Int16 / Int16Array
 */

void bolt_put_int16(struct BoltValue* value, int16_t x)
{
    _bolt_put(value, BOLT_INT16, 0, 0, &x, 1, sizeof(int16_t));
}

int16_t bolt_get_int16(const struct BoltValue* value)
{
    return value->data.as_int16[0];
}

void bolt_put_int16_array(struct BoltValue* value, int16_t* array, int32_t size)
{
    _bolt_put(value, BOLT_INT16, 0, 1, array, size, sizeof_n(int16_t, size));
}

int16_t bolt_get_int16_array_at(const struct BoltValue* value, int32_t index)
{
    return value->data.as_int16[index];
}


/*********************************************************************
 * Int32 / Int32Array
 */

void bolt_put_int32(struct BoltValue* value, int32_t x)
{
    _bolt_put(value, BOLT_INT32, 0, 0, &x, 1, sizeof(int32_t));
}

int32_t bolt_get_int32(const struct BoltValue* value)
{
    return value->data.as_int32[0];
}

void bolt_put_int32_array(struct BoltValue* value, int32_t* array, int32_t size)
{
    _bolt_put(value, BOLT_INT32, 0, 1, array, size, sizeof_n(int32_t, size));
}

int32_t bolt_get_int32_array_at(const struct BoltValue* value, int32_t index)
{
    return value->data.as_int32[index];
}


/*********************************************************************
 * Int64 / Int64Array
 */

void bolt_put_int64(struct BoltValue* value, int64_t x)
{
    _bolt_put(value, BOLT_INT64, 0, 0, &x, 1, sizeof(int64_t));
}

int64_t bolt_get_int64(const struct BoltValue* value)
{
    return value->data.as_int64[0];
}

void bolt_put_int64_array(struct BoltValue* value, int64_t* array, int32_t size)
{
    _bolt_put(value, BOLT_INT64, 0, 1, array, size, sizeof_n(int64_t, size));
}

int64_t bolt_get_int64_array_at(const struct BoltValue* value, int32_t index)
{
    return value->data.as_int64[index];
}


/*********************************************************************
 * Float32 / Float32Array
 */

void bolt_put_float32(struct BoltValue* value, float x)
{
    _bolt_put(value, BOLT_FLOAT32, 0, 0, &x, 1, sizeof(float));
}

float bolt_get_float32(const struct BoltValue* value)
{
    return value->data.as_float[0];
}

void bolt_put_float32_array(struct BoltValue* value, float* array, int32_t size)
{
    _bolt_put(value, BOLT_FLOAT32, 0, 1, array, size, sizeof_n(float, size));
}

float bolt_get_float32_array_at(const struct BoltValue* value, int32_t index)
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
    _bolt_put(value, BOLT_FLOAT64, 0, 0, &x, 1, sizeof(double));
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



//void bolt_put_structure(struct BoltValue* x, int16_t code, int32_t size);
//int16_t bolt_get_structure_code(struct BoltValue* x);
//struct BoltValue* bolt_get_structure_at(struct BoltValue* x, int32_t index);


#endif // SEABOLT_VALUES
