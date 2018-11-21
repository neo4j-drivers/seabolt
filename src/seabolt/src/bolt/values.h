/*
 * Copyright (c) 2002-2018 "Neo4j,"
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

/**
 * @file
 */

#ifndef SEABOLT_VALUES
#define SEABOLT_VALUES

#include <limits.h>
#include <stdio.h>
#include <stdint.h>

#include "bolt-public.h"

#if CHAR_BIT!=8
#error "Cannot compile if `char` is not 8-bit"
#endif

/**
 * Enumeration of the types available in the Bolt type system.
 */
enum BoltType {
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
typedef struct BoltValue BoltValue;

/**
 * Create a new BoltValue instance.
 *
 * @return
 */
SEABOLT_EXPORT struct BoltValue* BoltValue_create();

/**
 * Destroy a BoltValue instance.
 *
 * @param value
 */
SEABOLT_EXPORT void BoltValue_destroy(struct BoltValue* value);

/**
 * Duplicates a BoltValue instance
 *
 * @param value
 * @return
 */
SEABOLT_EXPORT struct BoltValue* BoltValue_duplicate(const struct BoltValue* value);

/**
 * Deep copy a BoltValue instance to another one
 *
 * @param src
 * @param dest
 * @return
 */
SEABOLT_EXPORT void BoltValue_copy(struct BoltValue* dest, const struct BoltValue* src);

/**
 * Return the size of a BoltValue.
 *
 * @param value
 * @return
 */
SEABOLT_EXPORT int32_t BoltValue_size(const struct BoltValue* value);

/**
 * Return the type of a BoltValue.
 *
 * @param value
 * @return
 */
SEABOLT_EXPORT enum BoltType BoltValue_type(const struct BoltValue* value);

SEABOLT_EXPORT int32_t BoltValue_to_string(const struct BoltValue* value, char *dest, int32_t length, struct BoltConnection* connection);
/**
 * Set a BoltValue instance to null.
 *
 * @param value
 */
SEABOLT_EXPORT void BoltValue_format_as_Null(struct BoltValue* value);

SEABOLT_EXPORT void BoltValue_format_as_Boolean(struct BoltValue* value, char data);

SEABOLT_EXPORT char BoltBoolean_get(const struct BoltValue* value);

SEABOLT_EXPORT void BoltValue_format_as_Integer(struct BoltValue* value, int64_t data);

SEABOLT_EXPORT int64_t BoltInteger_get(const struct BoltValue* value);

SEABOLT_EXPORT void BoltValue_format_as_Float(struct BoltValue* value, double data);

SEABOLT_EXPORT double BoltFloat_get(const struct BoltValue* value);

SEABOLT_EXPORT void BoltValue_format_as_String(struct BoltValue* value, const char* data, int32_t length);

SEABOLT_EXPORT char* BoltString_get(const struct BoltValue* value);

SEABOLT_EXPORT void BoltValue_format_as_Dictionary(struct BoltValue* value, int32_t length);

SEABOLT_EXPORT struct BoltValue* BoltDictionary_key(const struct BoltValue* value, int32_t index);

SEABOLT_EXPORT const char* BoltDictionary_get_key(const struct BoltValue* value, int32_t index);

SEABOLT_EXPORT int32_t BoltDictionary_get_key_size(const struct BoltValue* value, int32_t index);

SEABOLT_EXPORT int32_t
BoltDictionary_get_key_index(const struct BoltValue* value, const char* key, int32_t key_size, int32_t start_index);

SEABOLT_EXPORT int32_t BoltDictionary_set_key(struct BoltValue* value, int32_t index, const char* key, int32_t key_size);

SEABOLT_EXPORT struct BoltValue* BoltDictionary_value(const struct BoltValue* value, int32_t index);

SEABOLT_EXPORT struct BoltValue* BoltDictionary_value_by_key(const struct BoltValue* value, const char* key, int32_t key_size);

/**
 * Format a BoltValue instance to hold a list.
 *
 * @param value
 * @param length
 */
SEABOLT_EXPORT void BoltValue_format_as_List(struct BoltValue* value, int32_t length);

SEABOLT_EXPORT void BoltList_resize(struct BoltValue* value, int32_t size);

SEABOLT_EXPORT struct BoltValue* BoltList_value(const struct BoltValue* value, int32_t index);

SEABOLT_EXPORT void BoltValue_format_as_Bytes(struct BoltValue* value, char* data, int32_t length);

SEABOLT_EXPORT char BoltBytes_get(const struct BoltValue* value, int32_t index);

SEABOLT_EXPORT char* BoltBytes_get_all(const struct BoltValue* value);

SEABOLT_EXPORT void BoltValue_format_as_Structure(struct BoltValue* value, int16_t code, int32_t length);

SEABOLT_EXPORT int16_t BoltStructure_code(const struct BoltValue* value);

SEABOLT_EXPORT struct BoltValue* BoltStructure_value(const struct BoltValue* value, int32_t index);

#endif // SEABOLT_VALUES
