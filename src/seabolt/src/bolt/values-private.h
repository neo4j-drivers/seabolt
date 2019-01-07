/*
 * Copyright (c) 2002-2019 "Neo4j,"
 * Neo4j Sweden AB [http://neo4j.com]
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
#ifndef SEABOLT_VALUES_PRIVATE_H
#define SEABOLT_VALUES_PRIVATE_H

#include "values.h"
#include "string-builder.h"

typedef const char* (* name_resolver_func)(int16_t code);

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

/**
 * For holding extended values that exceed the size of a single BoltValue.
 */
union BoltExtendedValue {
    void* as_ptr;
    char* as_char;
    struct BoltValue* as_value;
};

/**
 * A BoltValue consists of a 128-bit header followed by a 128-byte data block. For
 * values that require more space than 128 bits, external memory is allocated and
 * a pointer to this is held in the inline data field.
 *
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
struct BoltValue {
    /// Type of this value, as defined in BoltType.
    int16_t type;

    /// Subtype tag, for use with Structure values.
    int16_t subtype;

    /// Logical size of this value.
    int32_t size;

    /// Physical size of this value, in bytes.
    uint64_t data_size;

    /// Data content of the value, or a pointer to
    /// extended content.
    union {
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

int BoltString_equals(struct BoltValue* value, const char* data, const size_t data_size);

/**
 * Write a textual representation of a BoltValue to a FILE.
 *
 * @param value
 * @param file
 * @param protocol_version
 * @return
 */
int
BoltValue_write(struct StringBuilder* builder, const struct BoltValue* value, name_resolver_func struct_name_resolver);

#endif //SEABOLT_VALUES_PRIVATE_H
