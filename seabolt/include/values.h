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

#include "config.h"

#if CHAR_BIT != 8
#error "Cannot compile if `char` is not 8-bit"
#endif

static const char HEX_DIGITS[] = {'0', '1', '2', '3', '4', '5', '6', '7',
                                  '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

#define hex7(mem, offset) HEX_DIGITS[((mem)[offset] >> 28) & 0x0F]

#define hex6(mem, offset) HEX_DIGITS[((mem)[offset] >> 24) & 0x0F]

#define hex5(mem, offset) HEX_DIGITS[((mem)[offset] >> 20) & 0x0F]

#define hex4(mem, offset) HEX_DIGITS[((mem)[offset] >> 16) & 0x0F]

#define hex3(mem, offset) HEX_DIGITS[((mem)[offset] >> 12) & 0x0F]

#define hex2(mem, offset) HEX_DIGITS[((mem)[offset] >> 8) & 0x0F]

#define hex1(mem, offset) HEX_DIGITS[((mem)[offset] >> 4) & 0x0F]

#define hex0(mem, offset) HEX_DIGITS[(mem)[offset] & 0x0F]


#define sizeof_n(type, n) (size_t)((n) >= 0 ? sizeof(type) * (n) : 0)


#define to_bit(x) (char)((x) == 0 ? 0 : 1);


struct array_t;

struct BoltValue;

enum BoltType
{
    /// Containers
    BOLT_NULL,                          /* Indicator of absence of value (Null in Bolt v1) */
    BOLT_LIST,                          /* Variable-length value container (List in Bolt v1) */

    /// Bits
    BOLT_BIT,                           /* ALSO IN BOLT v1 (as Boolean) */
    BOLT_BIT_ARRAY,
    BOLT_BYTE,
    BOLT_BYTE_ARRAY,                    /* ALSO IN BOLT v1 */

    /// Text
    BOLT_CHAR,                          /* Unicode (UTF-32) character */
    BOLT_CHAR_ARRAY,                    /* Array of Unicode (UTF-32) characters */
    BOLT_STRING,                        /* Unicode (UTF-8) string */
    BOLT_STRING_ARRAY,                  /* Array of Unicode (UTF-8) strings */
    BOLT_DICTIONARY,                    /* Sequence of Unicode (UTF-8) keys paired with values (similar to Map in Bolt v1) */

    /// Integers
    BOLT_INT8,
    BOLT_INT16,
    BOLT_INT32,
    BOLT_INT64,                         /* ALSO IN BOLT v1 (as Integer) */
    BOLT_INT8_ARRAY,
    BOLT_INT16_ARRAY,
    BOLT_INT32_ARRAY,
    BOLT_INT64_ARRAY,

    /// Floating point numbers
    BOLT_FLOAT64,                       /* ALSO IN BOLT v1 (as Float) */
    BOLT_FLOAT64_PAIR,
    BOLT_FLOAT64_TRIPLE,
    BOLT_FLOAT64_QUAD,
    BOLT_FLOAT64_ARRAY,
    BOLT_FLOAT64_PAIR_ARRAY,
    BOLT_FLOAT64_TRIPLE_ARRAY,
    BOLT_FLOAT64_QUAD_ARRAY,

    /// Structures
    BOLT_STRUCTURE,                     /* ALSO IN BOLT v1 (as Structure) */
    BOLT_STRUCTURE_ARRAY,

    /// Messages
    BOLT_MESSAGE,
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
    double* as_double;
    struct double_pair* as_double_pair;
    struct double_triple* as_double_triple;
    struct double_quad* as_double_quad;
    struct BoltValue* as_value;
    struct array_t* as_array;
};

struct array_t
{
    int32_t size;
    union data_t data;
};

// A BoltValue consists of a 16-byte header
// followed by a 16-byte data block
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
        double as_double[2];
        struct double_pair as_double_pair[1];
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
PUBLIC struct BoltValue* BoltValue_create();

/**
 * Set a BoltValue instance to null.
 *
 * @param value
 */
PUBLIC void BoltValue_to_Null(struct BoltValue* value);

/**
 * Reformat a BoltValue instance to hold a list.
 *
 * @param value
 * @param size
 */
PUBLIC void BoltValue_to_List(struct BoltValue* value, int32_t size);

PUBLIC void BoltValue_to_Bit(struct BoltValue* value, char x);

PUBLIC void BoltValue_to_Byte(struct BoltValue* value, char x);

PUBLIC void BoltValue_to_BitArray(struct BoltValue* value, char* array, int32_t size);

PUBLIC void BoltValue_to_ByteArray(struct BoltValue* value, char* array, int32_t size);

PUBLIC void BoltValue_to_Char(struct BoltValue * value, uint32_t x);

PUBLIC void BoltValue_to_CharArray(struct BoltValue * value, const uint32_t * array, int32_t size);

PUBLIC void BoltValue_to_String(struct BoltValue * value, const char * string, int32_t size);

PUBLIC void BoltValue_to_StringArray(struct BoltValue * value, int32_t size);

PUBLIC void BoltValue_to_Dictionary(struct BoltValue * value, int32_t size);

PUBLIC void BoltValue_to_Int8(struct BoltValue* value, int8_t x);

PUBLIC void BoltValue_to_Int16(struct BoltValue* value, int16_t x);

PUBLIC void BoltValue_to_Int32(struct BoltValue* value, int32_t x);

PUBLIC void BoltValue_to_Int64(struct BoltValue* value, int64_t x);

PUBLIC void BoltValue_to_Int8Array(struct BoltValue* value, int8_t* array, int32_t size);

PUBLIC void BoltValue_to_Int16Array(struct BoltValue* value, int16_t* array, int32_t size);

PUBLIC void BoltValue_to_Int32Array(struct BoltValue* value, int32_t* array, int32_t size);

PUBLIC void BoltValue_to_Int64Array(struct BoltValue* value, int64_t* array, int32_t size);

PUBLIC void BoltValue_to_Float64(struct BoltValue* value, double x);

PUBLIC void BoltValue_to_Float64Pair(struct BoltValue* value, struct double_pair x);

PUBLIC void BoltValue_to_Float64Triple(struct BoltValue* value, struct double_triple x);

PUBLIC void BoltValue_to_Float64Quad(struct BoltValue* value, struct double_quad x);

PUBLIC void BoltValue_to_Float64Array(struct BoltValue* value, double* array, int32_t size);

PUBLIC void BoltValue_to_Float64PairArray(struct BoltValue* value, struct double_pair * array, int32_t size);

PUBLIC void BoltValue_to_Float64TripleArray(struct BoltValue* value, struct double_triple * array, int32_t size);

PUBLIC void BoltValue_to_Float64QuadArray(struct BoltValue* value, struct double_quad * array, int32_t size);

PUBLIC void BoltValue_to_Structure(struct BoltValue* value, int16_t code, int32_t size);

PUBLIC void BoltValue_to_StructureArray(struct BoltValue* value, int16_t code, int32_t size);

PUBLIC void BoltValue_to_Message(struct BoltValue * value, int16_t code, int32_t size);

PUBLIC enum BoltType BoltValue_type(const struct BoltValue* value);

/**
 * Destroy a BoltValue instance.
 *
 * @param value
 */
PUBLIC void BoltValue_destroy(struct BoltValue* value);

PUBLIC int BoltValue_write(struct BoltValue * value, FILE * file, int32_t protocol_version);



PUBLIC int BoltNull_write(const struct BoltValue * value, FILE * file);


PUBLIC void BoltList_resize(struct BoltValue* value, int32_t size);

PUBLIC struct BoltValue* BoltList_value(const struct BoltValue* value, int32_t index);

PUBLIC int BoltList_write(const struct BoltValue * value, FILE * file, int32_t protocol_version);


PUBLIC char BoltBit_get(const struct BoltValue* value);

PUBLIC char BoltByte_get(const struct BoltValue* value);

PUBLIC char BoltBitArray_get(const struct BoltValue* value, int32_t index);

PUBLIC char BoltByteArray_get(const struct BoltValue* value, int32_t index);

PUBLIC char* BoltByteArray_get_all(struct BoltValue* value);

PUBLIC int BoltBit_write(const struct BoltValue * value, FILE * file);

PUBLIC int BoltBitArray_write(const struct BoltValue * value, FILE * file);

PUBLIC int BoltByte_write(const struct BoltValue * value, FILE * file);

PUBLIC int BoltByteArray_write(const struct BoltValue * value, FILE * file);



PUBLIC uint32_t BoltChar_get(const struct BoltValue * value);

PUBLIC uint32_t * BoltCharArray_get(struct BoltValue * value);

PUBLIC int BoltChar_write(const struct BoltValue * value, FILE * file);

PUBLIC int BoltCharArray_write(struct BoltValue * value, FILE * file);


PUBLIC char* BoltString_get(struct BoltValue * value);

PUBLIC char* BoltStringArray_get(struct BoltValue * value, int32_t index);

PUBLIC void BoltStringArray_put(struct BoltValue * value, int32_t index, const char * string, int32_t size);

PUBLIC int32_t BoltStringArray_get_size(struct BoltValue * value, int32_t index);

PUBLIC int BoltString_write(struct BoltValue * value, FILE * file);

PUBLIC int BoltStringArray_write(struct BoltValue * value, FILE * file);



PUBLIC struct BoltValue* BoltDictionary_key(struct BoltValue * value, int32_t index);

PUBLIC const char * BoltDictionary_get_key(struct BoltValue * value, int32_t index);

PUBLIC int32_t BoltDictionary_get_key_size(struct BoltValue * value, int32_t index);

PUBLIC int BoltDictionary_set_key(struct BoltValue * value, int32_t index, const char * key, size_t key_size);

PUBLIC struct BoltValue* BoltDictionary_value(struct BoltValue * value, int32_t index);

PUBLIC int BoltDictionary_write(struct BoltValue * value, FILE * file, int32_t protocol_version);



PUBLIC int8_t BoltInt8_get(const struct BoltValue* value);

PUBLIC int16_t BoltInt16_get(const struct BoltValue* value);

PUBLIC int32_t BoltInt32_get(const struct BoltValue* value);

PUBLIC int64_t BoltInt64_get(const struct BoltValue* value);

PUBLIC int8_t BoltInt8Array_get(const struct BoltValue* value, int32_t index);

PUBLIC int16_t BoltInt16Array_get(const struct BoltValue* value, int32_t index);

PUBLIC int32_t BoltInt32Array_get(const struct BoltValue* value, int32_t index);

PUBLIC int64_t BoltInt64Array_get(const struct BoltValue* value, int32_t index);

PUBLIC int BoltInt8_write(struct BoltValue * value, FILE * file);

PUBLIC int BoltInt16_write(struct BoltValue * value, FILE * file);

PUBLIC int BoltInt32_write(struct BoltValue * value, FILE * file);

PUBLIC int BoltInt64_write(struct BoltValue * value, FILE * file);

PUBLIC int BoltInt8Array_write(struct BoltValue * value, FILE * file);

PUBLIC int BoltInt16Array_write(struct BoltValue * value, FILE * file);

PUBLIC int BoltInt32Array_write(struct BoltValue * value, FILE * file);

PUBLIC int BoltInt64Array_write(struct BoltValue * value, FILE * file);

PUBLIC double BoltFloat64_get(const struct BoltValue* value);

PUBLIC struct double_pair BoltFloat64Pair_get(const struct BoltValue* value);

PUBLIC struct double_triple BoltFloat64Triple_get(const struct BoltValue* value);

PUBLIC struct double_quad BoltFloat64Quad_get(const struct BoltValue* value);

PUBLIC double BoltFloat64Array_get(const struct BoltValue* value, int32_t index);

PUBLIC struct double_pair BoltFloat64PairArray_get(const struct BoltValue * value, int32_t index);

PUBLIC struct double_triple BoltFloat64TripleArray_get(const struct BoltValue * value, int32_t index);

PUBLIC struct double_quad BoltFloat64QuadArray_get(const struct BoltValue * value, int32_t index);

PUBLIC int BoltFloat64_write(struct BoltValue * value, FILE * file);

PUBLIC int BoltFloat64Pair_write(struct BoltValue * value, FILE * file);

PUBLIC int BoltFloat64Triple_write(struct BoltValue * value, FILE * file);

PUBLIC int BoltFloat64Quad_write(struct BoltValue * value, FILE * file);

PUBLIC int BoltFloat64Array_write(struct BoltValue * value, FILE * file);

PUBLIC int BoltFloat64PairArray_write(struct BoltValue * value, FILE * file);

PUBLIC int BoltFloat64TripleArray_write(struct BoltValue * value, FILE * file);

PUBLIC int BoltFloat64QuadArray_write(struct BoltValue * value, FILE * file);


PUBLIC int16_t BoltStructure_code(const struct BoltValue* value);

PUBLIC int16_t BoltMessage_code(const struct BoltValue * value);

PUBLIC struct BoltValue* BoltStructure_value(const struct BoltValue* value, int32_t index);

PUBLIC struct BoltValue* BoltMessage_value(const struct BoltValue * value, int32_t index);

PUBLIC int32_t BoltStructureArray_get_size(const struct BoltValue* value, int32_t index);

PUBLIC void BoltStructureArray_set_size(struct BoltValue* value, int32_t index, int32_t size);

PUBLIC struct BoltValue* BoltStructureArray_at(const struct BoltValue* value, int32_t array_index, int32_t structure_index);

PUBLIC int BoltStructure_write(struct BoltValue * value, FILE * file, int32_t protocol_version);

PUBLIC int BoltStructureArray_write(struct BoltValue * value, FILE * file, int32_t protocol_version);

PUBLIC int BoltMessage_write(struct BoltValue * value, FILE * file, int32_t protocol_version);


#endif // SEABOLT_VALUES
