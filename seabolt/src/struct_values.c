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

#include <stdint.h>
#include <values.h>

#include "_values.h"


void BoltValue_toStructure(struct BoltValue* value, int16_t code, int32_t size)
{
    size_t unit_size = sizeof(struct BoltValue);
    size_t data_size = unit_size * size;
    _BoltValue_recycle(value);
    _BoltValue_allocate(value, data_size);
    memset(value->data.extended.as_char, 0, data_size);
    _BoltValue_setType(value, BOLT_STRUCTURE, 0, size);
    value->code = code;
}

int16_t BoltStructure_code(const struct BoltValue* value)
{
    assert(BoltValue_type(value) == BOLT_STRUCTURE);
    return value->code;
}

struct BoltValue* BoltStructure_at(const struct BoltValue* value, int32_t index)
{
    assert(BoltValue_type(value) == BOLT_STRUCTURE);
    return &value->data.extended.as_value[index];
}
