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
#include <values.h>

#include "_values.h"


struct BoltValue* BoltValue_create()
{
    size_t size = sizeof(struct BoltValue);
    struct BoltValue* value = BoltMem_allocate(size);
    _BoltValue_setType(value, BOLT_NULL, 0, 0);
    value->data_size = 0;
    value->data.as_uint64[0] = 0;
    value->data.extended.as_ptr = NULL;
    return value;
}

void BoltValue_toNull(struct BoltValue* value)
{
    _BoltValue_to(value, BOLT_NULL, 0, 0, NULL, 0);
}

void BoltValue_toList(struct BoltValue* value, int32_t size)
{
    size_t unit_size = sizeof(struct BoltValue);
    size_t data_size = unit_size * size;
    _BoltValue_recycle(value);
    _BoltValue_allocate(value, data_size);
    memset(value->data.extended.as_char, 0, data_size);
    _BoltValue_setType(value, BOLT_LIST, 0, size);
}

enum BoltType BoltValue_type(const struct BoltValue* value)
{
    return (enum BoltType)(value->type);
}

int BoltValue_isArray(const struct BoltValue* value)
{
    return value->is_array;
}

void BoltValue_destroy(struct BoltValue* value)
{
    BoltValue_toNull(value);
    BoltMem_deallocate(value, sizeof(struct BoltValue));
}

void BoltList_resize(struct BoltValue* value, int32_t size)
{
    assert(BoltValue_type(value) == BOLT_LIST);
    _BoltValue_resize(value, size, 1);
}

struct BoltValue* BoltList_at(const struct BoltValue* value, int32_t index)
{
    assert(BoltValue_type(value) == BOLT_LIST);
    return &value->data.extended.as_value[index];
}
