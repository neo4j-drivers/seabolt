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

    /// Control markers: 0x00..0x0F

    BOLT_NULL = 0x00,                           /* ALSO IN BOLT v1 (as Null) */
    BOLT_LIST = 0x01,                           /* ALSO IN BOLT v1 (as List) */
    BOLT_KEY_VALUE_LIST = 0x02,                 /* ALSO IN BOLT v1 (as Map) */
    BOLT_ERROR = 0x0E,
    BOLT_END = 0x0F,

    /// Bits and strings: 0x10..0x1F

    BOLT_BIT_0 = 0x10,
    BOLT_BIT_1 = 0x11,
    BOLT_BIT = 0x12,                            /* ALSO IN BOLT v1 (as Boolean) */
    BOLT_BYTE = 0x13,
    BOLT_CHAR16 = 0x14,
    BOLT_CHAR32 = 0x15,
    BOLT_UTF8 = 0x16,                           /* ALSO IN BOLT v1 (as String) */
    BOLT_UTF16 = 0x17,

    BOLT_BIT_0_ARRAY = 0x18,
    BOLT_BIT_1_ARRAY = 0x19,
    BOLT_BIT_ARRAY = 0x1A,
    BOLT_BYTE_ARRAY = 0x1B,                     /* ALSO IN BOLT v1 (as Bytes) */
    BOLT_CHAR16_ARRAY = 0x1C,
    BOLT_CHAR32_ARRAY = 0x1D,
    BOLT_UTF8_ARRAY = 0x1E,
    BOLT_UTF16_ARRAY = 0x1F,

    /// Numbers: 0x20..0x3F

    BOLT_NUM8 = 0x20,
    BOLT_NUM16 = 0x21,
    BOLT_NUM32 = 0x22,
    BOLT_NUM64 = 0x23,
    BOLT_INT8 = 0x24,
    BOLT_INT16 = 0x25,
    BOLT_INT32 = 0x26,
    BOLT_INT64 = 0x27,                          /* ALSO IN BOLT v1 (as Integer) */
    BOLT_FLOAT32 = 0x28,
    BOLT_FLOAT32_PAIR = 0x29,
    BOLT_FLOAT32_TRIPLE = 0x2A,
    BOLT_FLOAT32_QUAD = 0x2B,
    BOLT_FLOAT64 = 0x2C,                        /* ALSO IN BOLT v1 (as Float) */
    BOLT_FLOAT64_PAIR = 0x2D,
    BOLT_FLOAT64_TRIPLE = 0x2E,
    BOLT_FLOAT64_QUAD = 0x2F,

    BOLT_NUM8_ARRAY = 0x30,
    BOLT_NUM16_ARRAY = 0x31,
    BOLT_NUM32_ARRAY = 0x32,
    BOLT_NUM64_ARRAY = 0x33,
    BOLT_INT8_ARRAY = 0x34,
    BOLT_INT16_ARRAY = 0x35,
    BOLT_INT32_ARRAY = 0x36,
    BOLT_INT64_ARRAY = 0x37,
    BOLT_FLOAT32_ARRAY = 0x38,
    BOLT_FLOAT32_PAIR_ARRAY = 0x39,
    BOLT_FLOAT32_TRIPLE_ARRAY = 0x3A,
    BOLT_FLOAT32_QUAD_ARRAY = 0x3B,
    BOLT_FLOAT64_ARRAY = 0x3C,
    BOLT_FLOAT64_PAIR_ARRAY = 0x3D,
    BOLT_FLOAT64_TRIPLE_ARRAY = 0x3E,
    BOLT_FLOAT64_QUAD_ARRAY = 0x3F,

    /// Structures: 0x40..0x5F

    BOLT_STRUCT_0 = 0x40,                       /* ALSO IN BOLT v1 (as Structure) */
    BOLT_STRUCT_1 = 0x41,
    BOLT_STRUCT_2 = 0x42,
    BOLT_STRUCT_3 = 0x43,
    BOLT_STRUCT_4 = 0x44,
    BOLT_STRUCT_5 = 0x45,
    BOLT_STRUCT_6 = 0x46,
    BOLT_STRUCT_7 = 0x47,
    BOLT_STRUCT_8 = 0x48,
    BOLT_STRUCT_9 = 0x49,
    BOLT_STRUCT_A = 0x4A,
    BOLT_STRUCT_B = 0x4B,
    BOLT_STRUCT_C = 0x4C,
    BOLT_STRUCT_D = 0x4D,
    BOLT_STRUCT_E = 0x4E,
    BOLT_STRUCT_F = 0x4F,

    BOLT_STRUCT_0_ARRAY = 0x50,
    BOLT_STRUCT_1_ARRAY = 0x51,
    BOLT_STRUCT_2_ARRAY = 0x52,
    BOLT_STRUCT_3_ARRAY = 0x53,
    BOLT_STRUCT_4_ARRAY = 0x54,
    BOLT_STRUCT_5_ARRAY = 0x55,
    BOLT_STRUCT_6_ARRAY = 0x56,
    BOLT_STRUCT_7_ARRAY = 0x57,
    BOLT_STRUCT_8_ARRAY = 0x58,
    BOLT_STRUCT_9_ARRAY = 0x59,
    BOLT_STRUCT_A_ARRAY = 0x5A,
    BOLT_STRUCT_B_ARRAY = 0x5B,
    BOLT_STRUCT_C_ARRAY = 0x5C,
    BOLT_STRUCT_D_ARRAY = 0x5D,
    BOLT_STRUCT_E_ARRAY = 0x5E,
    BOLT_STRUCT_F_ARRAY = 0x5F,

    /// Control structures: 0x80..0xFF

    BOLT_REQUEST_0 = 0x80,  // requests match 10xxxxxx
    BOLT_SUMMARY_0 = 0xC0,  // summaries match 11xxxxxx

};

struct BoltValue
{
    uint8_t channel;
    uint8_t type;
    uint16_t code;
    int32_t logical_size;
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
        value->physical_size = physical_size;
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

void _bolt_put(struct BoltValue* value, uint8_t type, const void* data, int32_t logical_size, size_t physical_size)
{
    value->type = type;
    value->code = 0;
    value->logical_size = logical_size;
    _bolt_alloc(value, physical_size);
    _bolt_copy_data(value, data, 0, physical_size);
}


struct BoltValue bolt_value()
{
    struct BoltValue value;
    value.channel = 0;
    value.type = BOLT_NULL;
    value.logical_size = 0;
    value.physical_size = 0;
    value.data.as_ptr = NULL;
    return value;
}



/*********************************************************************
 * Null
 */

void bolt_put_null(struct BoltValue* value)
{
    _bolt_put(value, BOLT_NULL, NULL, 0, 0);
}



/*********************************************************************
 * Bit / BitArray
 */

void bolt_put_bit(struct BoltValue* value, char x)
{
    _bolt_put(value, BOLT_BIT, &x, 1, sizeof(char));
}

char bolt_get_bit(struct BoltValue* value)
{
    return to_bit(value->data.as_char[0]);
}

void bolt_put_bit_array(struct BoltValue* value, char* array, int32_t size)
{
    _bolt_put(value, BOLT_BIT_ARRAY, array, size, sizeof_n(char, size));
}

char bolt_get_bit_array_at(struct BoltValue* value, int32_t index)
{
    return to_bit(value->data.as_char[index]);
}



/*********************************************************************
 * Byte / ByteArray
 */

void bolt_put_byte(struct BoltValue* value, char x)
{
    _bolt_put(value, BOLT_BYTE, &x, 1, sizeof(char));
}

char bolt_get_byte(struct BoltValue* value)
{
    return value->data.as_char[0];
}

void bolt_put_byte_array(struct BoltValue* value, char* array, int32_t size)
{
    _bolt_put(value, BOLT_BYTE_ARRAY, array, size, sizeof_n(char, size));
}

char bolt_get_byte_array_at(struct BoltValue* value, int32_t index)
{
    return value->data.as_char[index];
}



/*********************************************************************
 * Char16 / Char16Array
 */

void bolt_put_char16(struct BoltValue* value, uint16_t x)
{
    _bolt_put(value, BOLT_CHAR16, &x, 1, sizeof(uint16_t));
}

void bolt_put_char16_array(struct BoltValue* value, uint16_t* array, int32_t size)
{
    _bolt_put(value, BOLT_CHAR16_ARRAY, array, size, sizeof_n(uint16_t, size));
}



/*********************************************************************
 * Char32 / Char32Array
 */

void bolt_put_char32(struct BoltValue* value, uint32_t x)
{
    _bolt_put(value, BOLT_CHAR32, &x, 1, sizeof(uint32_t));
}

void bolt_put_char32_array(struct BoltValue* value, uint32_t* array, int32_t size)
{
    _bolt_put(value, BOLT_CHAR32_ARRAY, array, size, sizeof_n(uint32_t, size));
}



/*********************************************************************
 * UTF8 / UTF8Array
 */

void bolt_put_utf8(struct BoltValue* value, char* string, int32_t size)
{
    _bolt_put(value, BOLT_UTF8, string, size, sizeof_n(char, size));
}

char* bolt_get_utf8(struct BoltValue* value)
{
    return value->data.as_char;
}

void bolt_put_utf8_array(struct BoltValue* value)
{
    _bolt_put(value, BOLT_UTF8_ARRAY, NULL, 0, 0);
}

void bolt_put_utf8_array_next(struct BoltValue* value, char* string, int32_t size)
{
    assert(value->type == BOLT_UTF8_ARRAY);
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
 * UTF16 / UTF16Array
 */

void bolt_put_utf16(struct BoltValue* value, uint16_t* string, int32_t size)
{
    _bolt_put(value, BOLT_UTF16, string, size, sizeof_n(uint16_t, size));
}



/*********************************************************************
 * Num8 / Num8Array
 */

void bolt_put_num8(struct BoltValue* value, uint8_t x)
{
    _bolt_put(value, BOLT_NUM8, &x, 1, sizeof(uint8_t));
}

uint8_t bolt_get_num8(struct BoltValue* value)
{
    return value->data.as_uint8[0];
}

void bolt_put_num8_array(struct BoltValue* value, uint8_t* array, int32_t size)
{
    _bolt_put(value, BOLT_NUM8_ARRAY, array, size, sizeof_n(uint8_t, size));
}

uint8_t bolt_get_num8_array_at(struct BoltValue* value, int32_t index)
{
    return value->data.as_uint8[index];
}



/*********************************************************************
 * Num16 / Num16Array
 */

void bolt_put_num16(struct BoltValue* value, uint16_t x)
{
    _bolt_put(value, BOLT_NUM16, &x, 1, sizeof(uint16_t));
}

uint16_t bolt_get_num16(struct BoltValue* value)
{
    return value->data.as_uint16[0];
}

void bolt_put_num16_array(struct BoltValue* value, uint16_t* array, int32_t size)
{
    _bolt_put(value, BOLT_NUM16_ARRAY, array, size, sizeof_n(uint16_t, size));
}

uint16_t bolt_get_num16_array_at(struct BoltValue* value, int32_t index)
{
    return value->data.as_uint16[index];
}



/*********************************************************************
 * Num32 / Num32Array
 */

void bolt_put_num32(struct BoltValue* value, uint32_t x)
{
    _bolt_put(value, BOLT_NUM32, &x, 1, sizeof(uint32_t));
}

uint32_t bolt_get_num32(struct BoltValue* value)
{
    return value->data.as_uint32[0];
}

void bolt_put_num32_array(struct BoltValue* value, uint32_t* array, int32_t size)
{
    _bolt_put(value, BOLT_NUM32_ARRAY, array, size, sizeof_n(uint32_t, size));
}

uint32_t bolt_get_num32_array_at(struct BoltValue* value, int32_t index)
{
    return value->data.as_uint32[index];
}



/*********************************************************************
 * Num64 / Num64Array
 */

void bolt_put_num64(struct BoltValue* value, uint64_t x)
{
    _bolt_put(value, BOLT_NUM64, &x, 1, sizeof(uint64_t));
}

uint64_t bolt_get_num64(struct BoltValue* value)
{
    return value->data.as_uint64[0];
}

void bolt_put_num64_array(struct BoltValue* value, uint64_t* array, int32_t size)
{
    _bolt_put(value, BOLT_NUM64_ARRAY, array, size, sizeof_n(uint64_t, size));
}

uint64_t bolt_get_num64_array_at(struct BoltValue* value, int32_t index)
{
    return value->data.as_uint64[index];
}



/*********************************************************************
 * Int8 / Int8Array
 */

void bolt_put_int8(struct BoltValue* value, int8_t x)
{
    _bolt_put(value, BOLT_INT8, &x, 1, sizeof(int8_t));
}

int8_t bolt_get_int8(struct BoltValue* value)
{
    return value->data.as_int8[0];
}

void bolt_put_int8_array(struct BoltValue* value, int8_t* array, int32_t size)
{
    _bolt_put(value, BOLT_INT8_ARRAY, array, size, sizeof_n(int8_t, size));
}

int8_t bolt_get_int8_array_at(struct BoltValue* value, int32_t index)
{
    return value->data.as_int8[index];
}



/*********************************************************************
 * Int16 / Int16Array
 */

void bolt_put_int16(struct BoltValue* value, int16_t x)
{
    _bolt_put(value, BOLT_INT16, &x, 1, sizeof(int16_t));
}

int16_t bolt_get_int16(struct BoltValue* value)
{
    return value->data.as_int16[0];
}

void bolt_put_int16_array(struct BoltValue* value, int16_t* array, int32_t size)
{
    _bolt_put(value, BOLT_INT16_ARRAY, array, size, sizeof_n(int16_t, size));
}

int16_t bolt_get_int16_array_at(struct BoltValue* value, int32_t index)
{
    return value->data.as_int16[index];
}



/*********************************************************************
 * Int32 / Int32Array
 */

void bolt_put_int32(struct BoltValue* value, int32_t x)
{
    _bolt_put(value, BOLT_INT32, &x, 1, sizeof(int32_t));
}

int32_t bolt_get_int32(struct BoltValue* value)
{
    return value->data.as_int32[0];
}

void bolt_put_int32_array(struct BoltValue* value, int32_t* array, int32_t size)
{
    _bolt_put(value, BOLT_INT32_ARRAY, array, size, sizeof_n(int32_t, size));
}

int32_t bolt_get_int32_array_at(struct BoltValue* value, int32_t index)
{
    return value->data.as_int32[index];
}



/*********************************************************************
 * Int64 / Int64Array
 */

void bolt_put_int64(struct BoltValue* value, int64_t x)
{
    _bolt_put(value, BOLT_INT64, &x, 1, sizeof(int64_t));
}

int64_t bolt_get_int64(struct BoltValue* value)
{
    return value->data.as_int64[0];
}

void bolt_put_int64_array(struct BoltValue* value, int64_t* array, int32_t size)
{
    _bolt_put(value, BOLT_INT64_ARRAY, array, size, sizeof_n(int64_t, size));
}

int64_t bolt_get_int64_array_at(struct BoltValue* value, int32_t index)
{
    return value->data.as_int64[index];
}



/*********************************************************************
 * Float32 / Float32Array
 */

void bolt_put_float32(struct BoltValue* value, float x)
{
    _bolt_put(value, BOLT_FLOAT32, &x, 1, sizeof(float));
}

float bolt_get_float32(struct BoltValue* value)
{
    return value->data.as_float[0];
}

void bolt_put_float32_array(struct BoltValue* value, float* array, int32_t size)
{
    _bolt_put(value, BOLT_FLOAT32_ARRAY, array, size, sizeof_n(float, size));
}

float bolt_get_float32_array_at(struct BoltValue* value, int32_t index)
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
    _bolt_put(value, BOLT_FLOAT64, &x, 1, sizeof(double));
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
