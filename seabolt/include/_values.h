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

#ifndef SEABOLT__VALUES
#define SEABOLT__VALUES

#include <assert.h>
#include <limits.h>
#include <memory.h>
#include <string.h>

#include "mem.h"
#include "values.h"

#define sizeof_n(type, n) (size_t)((n) >= 0 ? sizeof(type) * (n) : 0)

#define to_bit(x) (char)((x) == 0 ? 0 : 1);


/**
 * Allocate, reallocate or free memory for data storage.
 *
 * Since we recycle values, we can also potentially recycle
 * the dynamically-allocated storage.
 *
 * @param value the value in which to allocate storage
 * @param data_size the number of bytes of storage required
 */
void _BoltValue_allocate(struct BoltValue* value, size_t data_size);

void _BoltValue_copyData(struct BoltValue* value, const void* data, size_t offset, size_t length);

/**
 * Clean up a value for reuse.
 *
 * This sets any nested values to null.
 *
 * @param value
 */
void _BoltValue_recycle(struct BoltValue* value);

void _BoltValue_setType(struct BoltValue* value, enum BoltType type, char is_array, int size);

void _BoltValue_to(struct BoltValue* value, enum BoltType type, char is_array, int size,
                   const void* data, size_t data_size);


/**
 * Resize a value that contains multiple sub-values.
 *
 * @param value
 * @param size
 * @param multiplier
 */
void _BoltValue_resize(struct BoltValue* value, int32_t size, int multiplier);


#endif // SEABOLT__VALUES
