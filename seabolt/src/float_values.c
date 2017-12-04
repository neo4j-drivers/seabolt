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

#include <values.h>
#include "_values.h"


void BoltValue_toFloat32(struct BoltValue* value, float x)
{
    _BoltValue_to(value, BOLT_FLOAT32, 0, NULL, 1, 0);
    value->inline_data.as_float = x;
}

void BoltValue_toFloat64(struct BoltValue* value, double x)
{
    _BoltValue_to(value, BOLT_FLOAT64, 0, NULL, 1, 0);
    value->inline_data.as_double = x;
}

void BoltValue_toFloat32Array(struct BoltValue* value, float* array, int32_t size)
{
    _BoltValue_to(value, BOLT_FLOAT32, 1, array, size, sizeof_n(float, size));
}

float BoltFloat32_get(const struct BoltValue* value)
{
    return value->inline_data.as_float;
}

float BoltFloat32Array_get(const struct BoltValue* value, int32_t index)
{
    return value->data.as_float[index];
}

double BoltFloat64_get(struct BoltValue* value)
{
    return value->inline_data.as_double;
}
