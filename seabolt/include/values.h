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

/**
 * @file
 */

#ifndef SEABOLT_VALUES
#define SEABOLT_VALUES

#include <limits.h>
#include <stdio.h>
#include <stdint.h>

#if CHAR_BIT != 8
#error "Cannot compile if `char` is not 8-bit"
#endif

#define sizeof_n(type, n) (size_t)((n) >= 0 ? sizeof(type) * (n) : 0)

#define to_bit(x) (char)((x) == 0 ? 0 : 1);

struct array_t;

struct BoltValue;

enum BoltType
{
    /// Container types
    BOLT_NULL,                          /* Indicator of absence of value (Null in Bolt v1) */
    BOLT_LIST,                          /* Variable-length value container (List in Bolt v1) */

    /// Bit types
    BOLT_BIT,                           /* ALSO IN BOLT v1 (as Boolean) */
    BOLT_BYTE,
    BOLT_BIT_ARRAY,
    BOLT_BYTE_ARRAY,                    /* ALSO IN BOLT v1 */

    /// Text types
    BOLT_CHAR16,                        /* UTF-32 character */
    BOLT_CHAR32,                        /* UTF-32 character */
    BOLT_CHAR16_ARRAY,                  /* Array of UTF-16 characters */
    BOLT_CHAR32_ARRAY,                  /* Array of UTF-32 characters */
    BOLT_STRING8,                       /* UTF-8 string (String in Bolt v1) */
    BOLT_STRING16,                      /* UTF-16 string */
    BOLT_STRING8_ARRAY,                 /* Array of UTF-8 strings */
    BOLT_STRING16_ARRAY,                /* Array of UTF-16 strings */
    BOLT_DICTIONARY8,                   /* Sequence of UTF-8 keys paired with values (Map in Bolt v1) */
    BOLT_DICTIONARY16,                  /* Sequence of UTF-16 keys paired with values */

    /// Numeric types
    BOLT_NUM8,                          /* Unsigned 8-bit integer */
    BOLT_NUM16,
    BOLT_NUM32,
    BOLT_NUM64,
    BOLT_NUM8_ARRAY,
    BOLT_NUM16_ARRAY,
    BOLT_NUM32_ARRAY,
    BOLT_NUM64_ARRAY,
    BOLT_INT8,
    BOLT_INT16,
    BOLT_INT32,
    BOLT_INT64,                         /* ALSO IN BOLT v1 (as Integer) */
    BOLT_INT8_ARRAY,
    BOLT_INT16_ARRAY,
    BOLT_INT32_ARRAY,
    BOLT_INT64_ARRAY,
    BOLT_FLOAT32,
    BOLT_FLOAT32_PAIR,
    BOLT_FLOAT32_TRIPLE,
    BOLT_FLOAT32_QUAD,
    BOLT_FLOAT32_ARRAY,
    BOLT_FLOAT32_PAIR_ARRAY,
    BOLT_FLOAT32_TRIPLE_ARRAY,
    BOLT_FLOAT32_QUAD_ARRAY,
    BOLT_FLOAT64,                       /* ALSO IN BOLT v1 (as Float) */
    BOLT_FLOAT64_PAIR,
    BOLT_FLOAT64_TRIPLE,
    BOLT_FLOAT64_QUAD,
    BOLT_FLOAT64_ARRAY,
    BOLT_FLOAT64_PAIR_ARRAY,
    BOLT_FLOAT64_TRIPLE_ARRAY,
    BOLT_FLOAT64_QUAD_ARRAY,

    /// Composite types
    BOLT_STRUCTURE,                     /* ALSO IN BOLT v1 (as Structure) */
    BOLT_STRUCTURE_ARRAY,
    BOLT_REQUEST,
    BOLT_SUMMARY,
};

union data_t
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
    struct array_t* as_array;
};

struct array_t
{
    int32_t size;
    union data_t data;
};

struct BoltValue
{
    int16_t type;
    int16_t code;
    int32_t size;               // logical size
    size_t data_size;           // physical size
    union
    {
        char as_char[16];
        uint8_t as_uint8[16];
        uint16_t as_uint16[8];
        uint32_t as_uint32[4];
        uint64_t as_uint64[2];
        int8_t as_int8[16];
        int16_t as_int16[8];
        int32_t as_int32[4];
        int64_t as_int64[2];
        float as_float[4];
        double as_double[2];
        union data_t extended;
    } data;
};


/**
 * Clean up a value for reuse.
 *
 * This sets any nested values to null.
 *
 * @param value
 */
void _recycle(struct BoltValue* value);

void _set_type(struct BoltValue* value, enum BoltType type, int size);

void _format(struct BoltValue* value, enum BoltType type, int size, const void* data, size_t data_size);


/**
 * Resize a value that contains multiple sub-values.
 *
 * @param value
 * @param size
 * @param multiplier
 */
void _resize(struct BoltValue* value, int32_t size, int multiplier);


/**
 * Create a new BoltValue instance.
 *
 * @return
 */
BOLT_PUBLIC_API struct BoltValue* BoltValue_create();

/**
 * Set a BoltValue instance to null.
 *
 * @param value
 */
BOLT_PUBLIC_API void BoltValue_to_Null(struct BoltValue* value);

/**
 * Reformat a BoltValue instance to hold a list.
 *
 * @param value
 * @param size
 */
BOLT_PUBLIC_API void BoltValue_to_List(struct BoltValue* value, int32_t size);

BOLT_PUBLIC_API void BoltValue_to_Bit(struct BoltValue* value, char x);

BOLT_PUBLIC_API void BoltValue_to_Byte(struct BoltValue* value, char x);

BOLT_PUBLIC_API void BoltValue_to_BitArray(struct BoltValue* value, char* array, int32_t size);

BOLT_PUBLIC_API void BoltValue_to_ByteArray(struct BoltValue* value, char* array, int32_t size);

BOLT_PUBLIC_API void BoltValue_to_Char16(struct BoltValue* value, uint16_t x);

BOLT_PUBLIC_API void BoltValue_to_Char32(struct BoltValue* value, uint32_t x);

BOLT_PUBLIC_API void BoltValue_to_Char16Array(struct BoltValue* value, uint16_t* array, int32_t size);

BOLT_PUBLIC_API void BoltValue_to_Char32Array(struct BoltValue* value, uint32_t* array, int32_t size);

BOLT_PUBLIC_API void BoltValue_to_String8(struct BoltValue* value, const char* string, int32_t size);

BOLT_PUBLIC_API void BoltValue_to_String16(struct BoltValue* value, uint16_t* string, int32_t size);

BOLT_PUBLIC_API void BoltValue_to_String8Array(struct BoltValue* value, int32_t size);

BOLT_PUBLIC_API void BoltValue_to_String16Array(struct BoltValue* value, int32_t size);

BOLT_PUBLIC_API void BoltValue_to_Dictionary8(struct BoltValue* value, int32_t size);

BOLT_PUBLIC_API void BoltValue_to_Dictionary16(struct BoltValue* value, int32_t size);

BOLT_PUBLIC_API void BoltValue_to_Num8(struct BoltValue* value, uint8_t x);

BOLT_PUBLIC_API void BoltValue_to_Num16(struct BoltValue* value, uint16_t x);

BOLT_PUBLIC_API void BoltValue_to_Num32(struct BoltValue* value, uint32_t x);

BOLT_PUBLIC_API void BoltValue_to_Num64(struct BoltValue* value, uint64_t x);

BOLT_PUBLIC_API void BoltValue_to_Num8Array(struct BoltValue* value, uint8_t* array, int32_t size);

BOLT_PUBLIC_API void BoltValue_to_Num16Array(struct BoltValue* value, uint16_t* array, int32_t size);

BOLT_PUBLIC_API void BoltValue_to_Num32Array(struct BoltValue* value, uint32_t* array, int32_t size);

BOLT_PUBLIC_API void BoltValue_to_Num64Array(struct BoltValue* value, uint64_t* array, int32_t size);

BOLT_PUBLIC_API void BoltValue_to_Int8(struct BoltValue* value, int8_t x);

BOLT_PUBLIC_API void BoltValue_to_Int16(struct BoltValue* value, int16_t x);

BOLT_PUBLIC_API void BoltValue_to_Int32(struct BoltValue* value, int32_t x);

BOLT_PUBLIC_API void BoltValue_to_Int64(struct BoltValue* value, int64_t x);

BOLT_PUBLIC_API void BoltValue_to_Int8Array(struct BoltValue* value, int8_t* array, int32_t size);

BOLT_PUBLIC_API void BoltValue_to_Int16Array(struct BoltValue* value, int16_t* array, int32_t size);

BOLT_PUBLIC_API void BoltValue_to_Int32Array(struct BoltValue* value, int32_t* array, int32_t size);

BOLT_PUBLIC_API void BoltValue_to_Int64Array(struct BoltValue* value, int64_t* array, int32_t size);

BOLT_PUBLIC_API void BoltValue_to_Float32(struct BoltValue* value, float x);

BOLT_PUBLIC_API void BoltValue_to_Float32Pair(struct BoltValue* value, float x, float y);

BOLT_PUBLIC_API void BoltValue_to_Float32Triple(struct BoltValue* value, float x, float y, float z);

BOLT_PUBLIC_API void BoltValue_to_Float32Quad(struct BoltValue* value, float x, float y, float z, float a);

BOLT_PUBLIC_API void BoltValue_to_Float32Array(struct BoltValue* value, float* array, int32_t size);

BOLT_PUBLIC_API void BoltValue_to_Float32PairArray(struct BoltValue* value, int32_t size);

BOLT_PUBLIC_API void BoltValue_to_Float32TripleArray(struct BoltValue* value, int32_t size);

BOLT_PUBLIC_API void BoltValue_to_Float32QuadArray(struct BoltValue* value, int32_t size);

BOLT_PUBLIC_API void BoltValue_to_Float64(struct BoltValue* value, double x);

BOLT_PUBLIC_API void BoltValue_to_Float64Pair(struct BoltValue* value, double x, double y);

BOLT_PUBLIC_API void BoltValue_to_Float64Triple(struct BoltValue* value, double x, double y, double z);

BOLT_PUBLIC_API void BoltValue_to_Float64Quad(struct BoltValue* value, double x, double y, double z, double a);

BOLT_PUBLIC_API void BoltValue_to_Float64Array(struct BoltValue* value, double* array, int32_t size);

BOLT_PUBLIC_API void BoltValue_to_Float64PairArray(struct BoltValue* value, int32_t size);

BOLT_PUBLIC_API void BoltValue_to_Float64TripleArray(struct BoltValue* value, int32_t size);

BOLT_PUBLIC_API void BoltValue_to_Float64QuadArray(struct BoltValue* value, int32_t size);

BOLT_PUBLIC_API void BoltValue_to_Structure(struct BoltValue* value, int16_t code, int32_t size);

BOLT_PUBLIC_API void BoltValue_to_Request(struct BoltValue* value, int16_t code, int32_t size);

BOLT_PUBLIC_API void BoltValue_to_Summary(struct BoltValue* value, int16_t code, int32_t size);

BOLT_PUBLIC_API void BoltValue_to_StructureArray(struct BoltValue* value, int16_t code, int32_t size);

BOLT_PUBLIC_API enum BoltType BoltValue_type(const struct BoltValue* value);

/**
 * Destroy a BoltValue instance.
 *
 * @param value
 */
BOLT_PUBLIC_API void BoltValue_destroy(struct BoltValue* value);

BOLT_PUBLIC_API int BoltValue_write(struct BoltValue * value, FILE * file, int32_t protocol_version);



BOLT_PUBLIC_API int BoltNull_write(const struct BoltValue * value, FILE * file);


BOLT_PUBLIC_API void BoltList_resize(struct BoltValue* value, int32_t size);

BOLT_PUBLIC_API struct BoltValue* BoltList_value(const struct BoltValue* value, int32_t index);

BOLT_PUBLIC_API int BoltList_write(const struct BoltValue * value, FILE * file, int32_t protocol_version);


BOLT_PUBLIC_API char BoltBit_get(const struct BoltValue* value);

BOLT_PUBLIC_API char BoltByte_get(const struct BoltValue* value);

BOLT_PUBLIC_API char BoltBitArray_get(const struct BoltValue* value, int32_t index);

BOLT_PUBLIC_API char BoltByteArray_get(const struct BoltValue* value, int32_t index);

BOLT_PUBLIC_API char* BoltByteArray_get_all(struct BoltValue* value);

BOLT_PUBLIC_API int BoltBit_write(const struct BoltValue * value, FILE * file);

BOLT_PUBLIC_API int BoltBitArray_write(const struct BoltValue * value, FILE * file);

BOLT_PUBLIC_API int BoltByte_write(const struct BoltValue * value, FILE * file);

BOLT_PUBLIC_API int BoltByteArray_write(const struct BoltValue * value, FILE * file);



BOLT_PUBLIC_API uint16_t BoltChar16_get(const struct BoltValue* value);

BOLT_PUBLIC_API uint32_t BoltChar32_get(const struct BoltValue* value);

BOLT_PUBLIC_API uint16_t BoltChar16Array_get(const struct BoltValue* value, int32_t index);

BOLT_PUBLIC_API uint32_t BoltChar32Array_get(const struct BoltValue* value, int32_t index);



BOLT_PUBLIC_API char* BoltString8_get(struct BoltValue* value);

BOLT_PUBLIC_API uint16_t* BoltString16_get(struct BoltValue* value);

BOLT_PUBLIC_API char* BoltString8Array_get(struct BoltValue* value, int32_t index);

BOLT_PUBLIC_API uint16_t* BoltString16Array_get(struct BoltValue* value, int32_t index);

BOLT_PUBLIC_API void BoltString8Array_put(struct BoltValue* value, int32_t index, const char* string, int32_t size);

BOLT_PUBLIC_API void BoltString16Array_put(struct BoltValue* value, int32_t index, const uint16_t* string, int32_t size);

BOLT_PUBLIC_API int32_t BoltString8Array_get_size(struct BoltValue* value, int32_t index);

BOLT_PUBLIC_API int32_t BoltString16Array_get_size(struct BoltValue* value, int32_t index);

BOLT_PUBLIC_API int BoltString8_write(struct BoltValue * value, FILE * file);

BOLT_PUBLIC_API int BoltString8Array_write(struct BoltValue * value, FILE * file);



BOLT_PUBLIC_API struct BoltValue* BoltDictionary8_key(struct BoltValue* value, int32_t index);

BOLT_PUBLIC_API const char * BoltDictionary8_get_key(struct BoltValue* value, int32_t index);

BOLT_PUBLIC_API int32_t BoltDictionary8_get_key_size(struct BoltValue* value, int32_t index);

BOLT_PUBLIC_API struct BoltValue* BoltDictionary16_key(struct BoltValue* value, int32_t index);

BOLT_PUBLIC_API int BoltDictionary8_set_key(struct BoltValue* value, int32_t index, const char* key, size_t key_size);

BOLT_PUBLIC_API struct BoltValue* BoltDictionary16_with_key(struct BoltValue* value, int32_t index, const uint16_t* key,
                                            int32_t key_size);

BOLT_PUBLIC_API struct BoltValue* BoltDictionary8_value(struct BoltValue* value, int32_t index);

BOLT_PUBLIC_API struct BoltValue* BoltDictionary16_value(struct BoltValue* value, int32_t index);

BOLT_PUBLIC_API int BoltDictionary8_write(struct BoltValue * value, FILE * file, int32_t protocol_version);



BOLT_PUBLIC_API uint8_t BoltNum8_get(const struct BoltValue* value);

BOLT_PUBLIC_API uint16_t BoltNum16_get(const struct BoltValue* value);

BOLT_PUBLIC_API uint32_t BoltNum32_get(const struct BoltValue* value);

BOLT_PUBLIC_API uint64_t BoltNum64_get(const struct BoltValue* value);

BOLT_PUBLIC_API uint8_t BoltNum8Array_get(const struct BoltValue* value, int32_t index);

BOLT_PUBLIC_API uint16_t BoltNum16Array_get(const struct BoltValue* value, int32_t index);

BOLT_PUBLIC_API uint32_t BoltNum32Array_get(const struct BoltValue* value, int32_t index);

BOLT_PUBLIC_API uint64_t BoltNum64Array_get(const struct BoltValue* value, int32_t index);

BOLT_PUBLIC_API int BoltNum8_write(struct BoltValue * value, FILE * file);

BOLT_PUBLIC_API int BoltNum16_write(struct BoltValue * value, FILE * file);

BOLT_PUBLIC_API int BoltNum32_write(struct BoltValue * value, FILE * file);

BOLT_PUBLIC_API int BoltNum64_write(struct BoltValue * value, FILE * file);

BOLT_PUBLIC_API int BoltNum8Array_write(struct BoltValue * value, FILE * file);

BOLT_PUBLIC_API int BoltNum16Array_write(struct BoltValue * value, FILE * file);

BOLT_PUBLIC_API int BoltNum32Array_write(struct BoltValue * value, FILE * file);

BOLT_PUBLIC_API int BoltNum64Array_write(struct BoltValue * value, FILE * file);



BOLT_PUBLIC_API int8_t BoltInt8_get(const struct BoltValue* value);

BOLT_PUBLIC_API int16_t BoltInt16_get(const struct BoltValue* value);

BOLT_PUBLIC_API int32_t BoltInt32_get(const struct BoltValue* value);

BOLT_PUBLIC_API int64_t BoltInt64_get(const struct BoltValue* value);

BOLT_PUBLIC_API int8_t BoltInt8Array_get(const struct BoltValue* value, int32_t index);

BOLT_PUBLIC_API int16_t BoltInt16Array_get(const struct BoltValue* value, int32_t index);

BOLT_PUBLIC_API int32_t BoltInt32Array_get(const struct BoltValue* value, int32_t index);

BOLT_PUBLIC_API int64_t BoltInt64Array_get(const struct BoltValue* value, int32_t index);

BOLT_PUBLIC_API int BoltInt8_write(struct BoltValue * value, FILE * file);

BOLT_PUBLIC_API int BoltInt16_write(struct BoltValue * value, FILE * file);

BOLT_PUBLIC_API int BoltInt32_write(struct BoltValue * value, FILE * file);

BOLT_PUBLIC_API int BoltInt64_write(struct BoltValue * value, FILE * file);

BOLT_PUBLIC_API int BoltInt8Array_write(struct BoltValue * value, FILE * file);

BOLT_PUBLIC_API int BoltInt16Array_write(struct BoltValue * value, FILE * file);

BOLT_PUBLIC_API int BoltInt32Array_write(struct BoltValue * value, FILE * file);

BOLT_PUBLIC_API int BoltInt64Array_write(struct BoltValue * value, FILE * file);


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

BOLT_PUBLIC_API float BoltFloat32_get(const struct BoltValue* value);

BOLT_PUBLIC_API float BoltFloat32Array_get(const struct BoltValue* value, int32_t index);

BOLT_PUBLIC_API double BoltFloat64_get(const struct BoltValue* value);

BOLT_PUBLIC_API double BoltFloat64Array_get(const struct BoltValue* value, int32_t index);

BOLT_PUBLIC_API int BoltFloat32_write(struct BoltValue * value, FILE * file);

BOLT_PUBLIC_API int BoltFloat32Array_write(struct BoltValue * value, FILE * file);

BOLT_PUBLIC_API int BoltFloat64_write(struct BoltValue * value, FILE * file);

BOLT_PUBLIC_API int BoltFloat64Array_write(struct BoltValue * value, FILE * file);


BOLT_PUBLIC_API int16_t BoltStructure_code(const struct BoltValue* value);

BOLT_PUBLIC_API int16_t BoltRequest_code(const struct BoltValue* value);

BOLT_PUBLIC_API int16_t BoltSummary_code(const struct BoltValue* value);

BOLT_PUBLIC_API struct BoltValue* BoltStructure_value(const struct BoltValue* value, int32_t index);

BOLT_PUBLIC_API struct BoltValue* BoltRequest_value(const struct BoltValue* value, int32_t index);

BOLT_PUBLIC_API struct BoltValue* BoltSummary_value(const struct BoltValue* value, int32_t index);

BOLT_PUBLIC_API int32_t BoltStructureArray_get_size(const struct BoltValue* value, int32_t index);

BOLT_PUBLIC_API void BoltStructureArray_set_size(struct BoltValue* value, int32_t index, int32_t size);

BOLT_PUBLIC_API struct BoltValue* BoltStructureArray_at(const struct BoltValue* value, int32_t array_index, int32_t structure_index);

BOLT_PUBLIC_API int BoltStructure_write(struct BoltValue * value, FILE * file, int32_t protocol_version);

BOLT_PUBLIC_API int BoltStructureArray_write(struct BoltValue * value, FILE * file, int32_t protocol_version);

BOLT_PUBLIC_API int BoltRequest_write(struct BoltValue * value, FILE * file, int32_t protocol_version);

BOLT_PUBLIC_API int BoltSummary_write(struct BoltValue * value, FILE * file, int32_t protocol_version);


#endif // SEABOLT_VALUES
