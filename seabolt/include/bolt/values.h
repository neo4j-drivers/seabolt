/*
 * Copyright (c) 2002-2018 "Neo Technology,"
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

#define hex5(mem, offset) HEX_DIGITS[((mem)[offset] >> 20) & 0x0F]

#define hex4(mem, offset) HEX_DIGITS[((mem)[offset] >> 16) & 0x0F]

#define hex3(mem, offset) HEX_DIGITS[((mem)[offset] >> 12) & 0x0F]

#define hex2(mem, offset) HEX_DIGITS[((mem)[offset] >> 8) & 0x0F]

#define hex1(mem, offset) HEX_DIGITS[((mem)[offset] >> 4) & 0x0F]

#define hex0(mem, offset) HEX_DIGITS[(mem)[offset] & 0x0F]


#define sizeof_n(type, n) (size_t)((n) >= 0 ? sizeof(type) * (n) : 0)


#define to_bit(x) (char)((x) == 0 ? 0 : 1);


struct BoltValue;

/**
 * Enumeration of the types available in the Bolt type system.
 */
enum BoltType
{
    BOLT_NULL = 0,
    BOLT_BOOLEAN = 1,
    BOLT_INTEGER = 2,
    BOLT_FLOAT = 3,
    BOLT_STRING = 4,
    BOLT_DICTIONARY = 5,
    BOLT_LIST = 6,
    BOLT_BYTES = 7,
    BOLT_STRUCTURE = 8,
};

/**
 * For holding extended values that exceed the size of a single BoltValue.
 */
union BoltExtendedValue
{
    void * as_ptr;
    char * as_char;
    struct BoltValue * as_value;
};

/**
 * A BoltValue consists of a 128-bit header followed by a 128-byte data block. For
 * values that require more space than 128 bits, external memory is allocated and
 * a pointer to this is held in the inline data field.
 * ```
 * +----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+
 * |  type   | subtype |  (logical) size   |         (physical) data size          |
 * |[16 bits]|[16 bits]|     [32 bits]     |               [64 bits]               |
 * +----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+
 * |                      inline data or pointer to external data                  |
 * |                                  [128 bits]                                   |
 * +----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+
 * ```
 */
struct BoltValue
{
    /// Type of this value, as defined in BoltType.
    int16_t type;

    /// Subtype tag, for use with Structure values.
    int16_t subtype;

    /// Logical size of this value.
    /// For portability between platforms, the logical
    /// size of a value (e.g. string length or list size)
    /// cannot exceed 2^31, therefore an int32_t is safe
    /// here.
    int32_t size;

    /// Physical size of this value, in bytes.
    uint64_t data_size;

    /// Data content of the value, or a pointer to
    /// extended content.
    union
    {
        char as_char[16];
        uint32_t as_uint32[4];
        int8_t as_int8[16];
        int16_t as_int16[8];
        int32_t as_int32[4];
        int64_t as_int64[2];
        double as_double[2];
        union BoltExtendedValue extended;
    } data;

};



/**
 * Create a new BoltValue instance.
 *
 * @return
 */
PUBLIC struct BoltValue* BoltValue_create();

/**
 * Destroy a BoltValue instance.
 *
 * @param value
 */
PUBLIC void BoltValue_destroy(struct BoltValue* value);

/**
 * Return the type of a BoltValue.
 *
 * @param value
 * @return
 */
PUBLIC enum BoltType BoltValue_type(const struct BoltValue * value);

/**
 * Write a textual representation of a BoltValue to a FILE.
 *
 * @param value
 * @param file
 * @param protocol_version
 * @return
 */
PUBLIC int BoltValue_write(struct BoltValue * value, FILE * file, int32_t protocol_version);



/**
 * Set a BoltValue instance to null.
 *
 * @param value
 */
PUBLIC void BoltValue_format_as_Null(struct BoltValue * value);



PUBLIC void BoltValue_format_as_Boolean(struct BoltValue * value, char data);

PUBLIC char BoltBoolean_get(const struct BoltValue * value);



PUBLIC void BoltValue_format_as_Integer(struct BoltValue * value, int64_t data);

PUBLIC int64_t BoltInteger_get(const struct BoltValue * value);



PUBLIC void BoltValue_format_as_Float(struct BoltValue * value, double data);

PUBLIC double BoltFloat_get(const struct BoltValue * value);



PUBLIC void BoltValue_format_as_String(struct BoltValue * value, const char * data, int32_t length);

PUBLIC char* BoltString_get(struct BoltValue * value);



PUBLIC void BoltValue_format_as_Dictionary(struct BoltValue * value, int32_t length);

PUBLIC struct BoltValue * BoltDictionary_key(struct BoltValue * value, int32_t index);

PUBLIC const char * BoltDictionary_get_key(struct BoltValue * value, int32_t index);

PUBLIC int32_t BoltDictionary_get_key_size(struct BoltValue * value, int32_t index);

PUBLIC int BoltDictionary_set_key(struct BoltValue * value, int32_t index, const char * key, size_t key_size);

PUBLIC struct BoltValue * BoltDictionary_value(struct BoltValue * value, int32_t index);



/**
 * Format a BoltValue instance to hold a list.
 *
 * @param value
 * @param length
 */
PUBLIC void BoltValue_format_as_List(struct BoltValue * value, int32_t length);

PUBLIC void BoltList_resize(struct BoltValue* value, int32_t size);

PUBLIC struct BoltValue * BoltList_value(const struct BoltValue* value, int32_t index);



PUBLIC void BoltValue_format_as_Bytes(struct BoltValue * value, char * data, int32_t length);

PUBLIC char BoltBytes_get(const struct BoltValue * value, int32_t index);

PUBLIC char * BoltBytes_get_all(struct BoltValue * value);



PUBLIC void BoltValue_format_as_Structure(struct BoltValue * value, int16_t code, int32_t length);

PUBLIC int16_t BoltStructure_code(const struct BoltValue* value);

PUBLIC struct BoltValue * BoltStructure_value(const struct BoltValue* value, int32_t index);



#endif // SEABOLT_VALUES
