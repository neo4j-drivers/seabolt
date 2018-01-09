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
#include <string.h>
#include <values.h>
#include <assert.h>


void BoltValue_to_Float32(struct BoltValue* value, float x)
{
    _format(value, BOLT_FLOAT32, 1, NULL, 0);
    value->data.as_float[0] = x;
}

void BoltValue_to_Float64(struct BoltValue* value, double x)
{
    _format(value, BOLT_FLOAT64, 1, NULL, 0);
    value->data.as_double[0] = x;
}

void BoltValue_to_Float32Array(struct BoltValue* value, float* array, int32_t size)
{
    if (size <= sizeof(value->data) / sizeof(float))
    {
        _format(value, BOLT_FLOAT32_ARRAY, size, NULL, 0);
        memcpy(value->data.as_float, array, (size_t)(size));
    }
    else
    {
        _format(value, BOLT_FLOAT32_ARRAY, size, array, sizeof_n(float, size));
    }
}

void BoltValue_to_Float64Array(struct BoltValue* value, double* array, int32_t size)
{
    if (size <= sizeof(value->data) / sizeof(double))
    {
        _format(value, BOLT_FLOAT64_ARRAY, size, NULL, 0);
        memcpy(value->data.as_double, array, (size_t)(size));
    }
    else
    {
        _format(value, BOLT_FLOAT64_ARRAY, size, array, sizeof_n(double, size));
    }
}

float BoltFloat32_get(const struct BoltValue* value)
{
    return value->data.as_float[0];
}

double BoltFloat64_get(const struct BoltValue* value)
{
    return value->data.as_double[0];
}

float BoltFloat32Array_get(const struct BoltValue* value, int32_t index)
{
    const float* data = value->size <= sizeof(value->data) / sizeof(float) ?
                        value->data.as_float : value->data.extended.as_float;
    return data[index];
}

double BoltFloat64Array_get(const struct BoltValue* value, int32_t index)
{
    const double* data = value->size <= sizeof(value->data) / sizeof(double) ?
                         value->data.as_double : value->data.extended.as_double;
    return data[index];
}

int BoltFloat32_write(struct BoltValue * value, FILE * file)
{
    assert(BoltValue_type(value) == BOLT_FLOAT32);
    fprintf(file, "f32(%f)", BoltFloat32_get(value));
    return 0;
}

int BoltFloat32Array_write(struct BoltValue * value, FILE * file)
{
    assert(BoltValue_type(value) == BOLT_FLOAT32_ARRAY);
    fprintf(file, "f32[");
    for (int i = 0; i < value->size; i++)
    {
        fprintf(file, i == 0 ? "%f" : ", %f", BoltFloat32Array_get(value, i));
    }
    fprintf(file, "]");
    return 0;
}

int BoltFloat64_write(struct BoltValue * value, FILE * file)
{
    assert(BoltValue_type(value) == BOLT_FLOAT64);
    fprintf(file, "f64(%f)", BoltFloat64_get(value));
    return 0;
}

int BoltFloat64Array_write(struct BoltValue * value, FILE * file)
{
    assert(BoltValue_type(value) == BOLT_FLOAT64_ARRAY);
    fprintf(file, "f64[");
    for (int i = 0; i < value->size; i++)
    {
        fprintf(file, i == 0 ? "%f" : ", %f", BoltFloat64Array_get(value, i));
    }
    fprintf(file, "]");
    return 0;
}
