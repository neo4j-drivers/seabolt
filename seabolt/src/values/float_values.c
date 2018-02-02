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
#include "mem.h"

void BoltValue_to_Float64(struct BoltValue* value, double data)
{
    BoltValue_to_Float64Tuple(value, &data, 1);
}

void BoltValue_to_Float64Tuple(struct BoltValue* value, double * data, int16_t width)
{
    if (width <= 0)
    {
        BoltValue_to_Null(value);
    }
    else if (width == 1)
    {
        _format(value, BOLT_FLOAT64, 1, 1, NULL, 0);
        value->data.as_double[0] = data[0];
    }
    else
    {
        size_t data_size = sizeof_n(double, width);
        if (data_size <= sizeof(value->data))
        {
            _format(value, BOLT_FLOAT64, width, 1, NULL, 0);
            memcpy(value->data.as_double, data, data_size);
        }
        else
        {
            _format(value, BOLT_FLOAT64, width, 1, data, data_size);
        }
    }
}

void BoltValue_to_Float64Array(struct BoltValue* value, double * data, int32_t length)
{
    BoltValue_to_Float64TupleArray(value, data, 1, length);
}

void BoltValue_to_Float64TupleArray(struct BoltValue* value, double * data, int16_t width, int32_t length)
{
    if (width <= 0)
    {
        BoltValue_to_Null(value);
    }
    else if (width == 1)
    {
        if (length <= sizeof(value->data) / sizeof(double))
        {
            _format(value, BOLT_FLOAT64_ARRAY, -1, length, NULL, 0);
            memcpy(value->data.as_double, data, (size_t)(length));
        }
        else
        {
            _format(value, BOLT_FLOAT64_ARRAY, -1, length, data, sizeof_n(double, length));
        }
    }
    else
    {
        size_t data_size = sizeof_n(double, width * length);
        _format(value, BOLT_FLOAT64_ARRAY, width, length, data, data_size);
    }
}

double BoltFloat64_get(const struct BoltValue* value)
{
    return value->data.as_double[0];
}

double BoltFloat64Tuple_get(const struct BoltValue* value, int16_t offset)
{
    int16_t width = value->subtype;
    size_t data_size = sizeof_n(double, width);
    const double * data = data_size <= sizeof(value->data) ?
                          value->data.as_double : value->data.extended.as_double;
    return data[offset];
}

double BoltFloat64Array_get(const struct BoltValue* value, int32_t index)
{
    const double* data = value->size <= sizeof(value->data) / sizeof(double) ?
                         value->data.as_double : value->data.extended.as_double;
    return data[index];
}

double BoltFloat64TupleArray_get(const struct BoltValue * value, int32_t index, int16_t offset)
{
    int16_t width = value->subtype;
    return value->data.extended.as_double[index * width + offset];
}

int BoltFloat64_write(struct BoltValue * value, FILE * file)
{
    assert(BoltValue_type(value) == BOLT_FLOAT64);
    int width = value->subtype;
    fprintf(file, "f64(");
    for (int16_t i = 0; i < width; i++)
    {
        if (i > 0) { fprintf(file, " "); }
        fprintf(file, "%f", BoltFloat64Tuple_get(value, i));
    }
    fprintf(file, ")");
    return 0;
}

int BoltFloat64Array_write(struct BoltValue * value, FILE * file)
{
    assert(BoltValue_type(value) == BOLT_FLOAT64_ARRAY);
    int width = value->subtype;
    fprintf(file, "f64[");
    for (int i = 0; i < value->size; i++)
    {
        if (i > 0) { fprintf(file, ", "); }
        for (int j = 0; j < width; j++)
        {
            if (j > 0) { fprintf(file, " "); }
            fprintf(file, "%f", BoltFloat64TupleArray_get(value, i, j));
        }
    }
    fprintf(file, "]");
    return 0;
}
