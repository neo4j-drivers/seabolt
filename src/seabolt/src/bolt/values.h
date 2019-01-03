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
 * The list of types available in the Bolt type system.
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
 * The type that holds one of the supported values as defined by \ref BoltType.
 */
typedef struct BoltValue BoltValue;

/**
 * Creates a new instance of \ref BoltValue.
 *
 * @return the pointer to the newly allocated \ref BoltValue instance.
 */
SEABOLT_EXPORT BoltValue* BoltValue_create();

/**
 * Destroys the passed \ref BoltValue instance.
 *
 * @param value the instance to be destroyed.
 */
SEABOLT_EXPORT void BoltValue_destroy(BoltValue* value);

/**
 * Duplicates the passed \ref BoltValue instance.
 *
 * It's the caller's responsibility to destroy the returned instance.
 *
 * @param value the instance to be duplicated.
 * @returns a pointer to the newly allocated and populated \ref BoltValue instance.
 */
SEABOLT_EXPORT BoltValue* BoltValue_duplicate(const BoltValue* value);

/**
 * Deep copies the passed \ref BoltValue instance to another one.
 *
 * Any existing information present in the _dest_ instance will be cleared out.
 *
 * @param dest the destination instance.
 * @param src the source instance.
 */
SEABOLT_EXPORT void BoltValue_copy(BoltValue* dest, const BoltValue* src);

/**
 * Returns the size of the passed \ref BoltValue instance.
 *
 * @param value the instance to be queried.
 * @returns the size.
 */
SEABOLT_EXPORT int32_t BoltValue_size(const BoltValue* value);

/**
 * Returns the type of the passed \ref BoltValue instance.
 *
 * @param value the instance to be queried.
 * @returns the type.
 */
SEABOLT_EXPORT enum BoltType BoltValue_type(const BoltValue* value);

/**
 * Copies the string representation of the passed \ref BoltValue instance to the provided buffers.
 *
 * @param value the instance to be stringified.
 * @param dest the destination buffer to copy the string representation. If no truncation occurs due to the
 *              buffer size being smaller than required, the string written will be terminated by '\0'.
 * @param length the length of the destination buffer.
 * @param connection the optional connection instance related with this \ref BoltValue. This is only useful for
 *          \ref BOLT_STRUCTURE typed values and the known pretty names (Node, Relation, Duration, etc.) will be
 *          printed rather than its type code.
 * @return the number of characters written to the destination buffer. If returned value is > length, it means that
 *          the string representation is truncated and not terminated by '\0'. Another call with an adequate sized
 *          buffer will ensure the full string represention to be available to the caller.
 */
SEABOLT_EXPORT int32_t
BoltValue_to_string(const BoltValue* value, char* dest, int32_t length, BoltConnection* connection);

/**
 * Sets the passed \ref BoltValue instance to null.
 *
 * @param value the instance to be updated
 */
SEABOLT_EXPORT void BoltValue_format_as_Null(BoltValue* value);

/**
 * Sets the passed \ref BoltValue instance to \ref BOLT_BOOLEAN.
 *
 * @param value the instance to be updated
 * @param data the boolean value to set, 1 for TRUE, 0 for FALSE.
 */
SEABOLT_EXPORT void BoltValue_format_as_Boolean(BoltValue* value, char data);

/**
 * Gets the boolean value stored in the passed \ref BoltValue instance.
 *
 * @param value the instance to be queried
 * @returns 1 for TRUE, 0 for FALSE.
 */
SEABOLT_EXPORT char BoltBoolean_get(const BoltValue* value);

/**
 * Sets the passed \ref BoltValue instance to \ref BOLT_INTEGER.
 *
 * @param value the instance to be updated
 * @param data the integer value to set.
 */
SEABOLT_EXPORT void BoltValue_format_as_Integer(BoltValue* value, int64_t data);

/**
 * Gets the integer value stored in the passed \ref BoltValue instance.
 *
 * @param value the instance to be queried
 * @returns the integer value stored.
 */
SEABOLT_EXPORT int64_t BoltInteger_get(const BoltValue* value);

/**
 * Sets the passed \ref BoltValue instance to \ref BOLT_FLOAT.
 *
 * @param value the instance to be updated
 * @param data the float value to set.
 */
SEABOLT_EXPORT void BoltValue_format_as_Float(BoltValue* value, double data);

/**
 * Gets the float value stored in the passed \ref BoltValue instance.
 *
 * @param value the instance to be queried
 * @returns the float value stored.
 */
SEABOLT_EXPORT double BoltFloat_get(const BoltValue* value);

/**
 * Sets the passed \ref BoltValue instance to \ref BOLT_STRING.
 *
 * @param value the instance to be updated
 * @param data the string buffer containing the value to be set.
 * @param length the length of the string buffer.
 */
SEABOLT_EXPORT void BoltValue_format_as_String(BoltValue* value, const char* data, int32_t length);

/**
 * Gets the string buffer stored in the passed \ref BoltValue instance.
 *
 * @param value the instance to be queried
 * @returns the string buffer. Actual length of the string can be queried by a call to \ref BoltValue_size.
 */
SEABOLT_EXPORT char* BoltString_get(const BoltValue* value);

/**
 * Sets the passed \ref BoltValue instance to \ref BOLT_DICTIONARY.
 *
 * @param value the instance to be updated
 * @param length the number of entries.
 */
SEABOLT_EXPORT void BoltValue_format_as_Dictionary(BoltValue* value, int32_t length);

/**
 * Returns an instance to a \ref BoltValue identifying the _key_ at _index_.
 *
 * @param value the instance to be queried
 * @param index the index of the key.
 * @returns \ref BoltValue instance identifying the key.
 */
SEABOLT_EXPORT BoltValue* BoltDictionary_key(const BoltValue* value, int32_t index);

/**
 * Returns the string buffer identifying the _key_ at _index_.
 *
 * @param value the instance to be queried
 * @param index the index of the key.
 * @returns the string buffer identifying the key.
 */
SEABOLT_EXPORT const char* BoltDictionary_get_key(const BoltValue* value, int32_t index);

/**
 * Returns the size of the string buffer identifying the _key_ at _index_.
 *
 * @param value the instance to be queried
 * @param index the index of the key.
 * @returns size of the string buffer identifying the key.
 */
SEABOLT_EXPORT int32_t BoltDictionary_get_key_size(const BoltValue* value, int32_t index);

/**
 * Returns the index of a _key_ if it is present in the passed \ref BoltValue instance.
 *
 * @param value the instance to be queried.
 * @param key the string buffer identifying the key to be searched for.
 * @param key_size the size of the string buffer identifying the key to be searched for.
 * @param start_index the start index of the actual search.
 * @returns the index of the key found, or -1 if not found.
 */
SEABOLT_EXPORT int32_t
BoltDictionary_get_key_index(const BoltValue* value, const char* key, int32_t key_size, int32_t start_index);

/**
 * Sets the _key_ value at _index_ from the passed in string buffer.
 *
 * @param value the instance to be updated.
 * @param index the index of the key to be set.
 * @param key the string buffer identifying the key to be set.
 * @param key_size the size of the string buffer identifying the key to be set.
 * @return 0 if successful, -1 if index is larger than the maximum value (INT32_MAX).
 */
SEABOLT_EXPORT int32_t
BoltDictionary_set_key(BoltValue* value, int32_t index, const char* key, int32_t key_size);

/**
 * Returns an instance to a \ref BoltValue identifying the _value_ at _index_.
 *
 * @param value the instance to be queried
 * @param index the index of the value.
 * @returns \ref BoltValue instance identifying the value, NULL if the index is out of bounds.
 */
SEABOLT_EXPORT BoltValue* BoltDictionary_value(const BoltValue* value, int32_t index);

/**
 * Returns an instance to a \ref BoltValue identifying the _value_ corresponding to the key passed as a
 * string buffer.
 *
 * @param value the instance to be queried
 * @param key the string buffer identifying the key to be searched.
 * @param key_size the size of the string buffer identifying the key to be searched.
 * @returns \ref BoltValue instance identifying the value, NULL if the key is not found.
 */
SEABOLT_EXPORT BoltValue*
BoltDictionary_value_by_key(const BoltValue* value, const char* key, int32_t key_size);

/**
 * Sets the passed \ref BoltValue instance to \ref BOLT_LIST.
 *
 * @param value the instance to be updated
 * @param length the number of entries.
 */
SEABOLT_EXPORT void BoltValue_format_as_List(BoltValue* value, int32_t length);

/**
 * Resizes the passed \ref BoltValue "list" instance.
 *
 * @param value the instance to be resized.
 * @param size the new size to be set. If it is smaller than the instance's current size, the
 *      elements at indices larger than size will be trimmed.
 */
SEABOLT_EXPORT void BoltList_resize(BoltValue* value, int32_t size);

/**
 * Returns an instance to a \ref BoltValue identifying the _value_ at _index_.
 *
 * @param value the instance to be queried
 * @param index the index of the value.
 * @returns \ref BoltValue instance identifying the value, NULL if the index is out of bounds.
 */
SEABOLT_EXPORT BoltValue* BoltList_value(const BoltValue* value, int32_t index);

/**
 * Sets the passed \ref BoltValue instance to \ref BOLT_BYTES.
 *
 * @param value the instance to be updated
 * @param data the byte buffer containing the value to be set.
 * @param length the length of the byte buffer.
 */
SEABOLT_EXPORT void BoltValue_format_as_Bytes(BoltValue* value, char* data, int32_t length);

/**
 * Returns the byte _value_ at _index_.
 *
 * @param value the instance to be queried
 * @param index the index of the byte to be queried
 * @returns the byte value (undefined if _index_ is out of bounds).
 */
SEABOLT_EXPORT char BoltBytes_get(const BoltValue* value, int32_t index);

/**
 * Returns the whole byte buffer stored in the passed \ref BoltValue.
 *
 * @param value the instance to be queried
 * @returns the stored byte buffer. Actual length of the byte buffer can be queried by a call
 *          to \ref BoltValue_size.
 */
SEABOLT_EXPORT char* BoltBytes_get_all(const BoltValue* value);

/**
 * Sets the passed \ref BoltValue instance to \ref BOLT_STRUCTURE.
 *
 * @param value the instance to be updated
 * @param code the code of the structure.
 * @param length the number of entries to be placed in the structure.
 */
SEABOLT_EXPORT void BoltValue_format_as_Structure(BoltValue* value, int16_t code, int32_t length);

/**
 * Returns the code of the structure.
 *
 * @param value the instance to be queried.
 * @return the code of the structure.
 */
SEABOLT_EXPORT int16_t BoltStructure_code(const BoltValue* value);

/**
 * Returns an instance to a \ref BoltValue identifying the _entry_ at _index_.
 *
 * @param value the instance to be queried
 * @param index the index of the entry.
 * @returns \ref BoltValue instance identifying the entry at _index_, NULL if the index is out of bounds.
 */
SEABOLT_EXPORT BoltValue* BoltStructure_value(const BoltValue* value, int32_t index);

#endif // SEABOLT_VALUES
