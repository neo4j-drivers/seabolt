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

void BoltValue_to_Float64(struct BoltValue* value, double x)
{
    _format(value, BOLT_FLOAT64, 1, NULL, 0);
    value->data.as_double[0] = x;
}

void BoltValue_to_Float64Pair(struct BoltValue* value, struct double_pair x)
{
    _format(value, BOLT_FLOAT64_PAIR, 1, NULL, 0);
    value->data.as_double_pair[0] = x;
}

void BoltValue_to_Float64Triple(struct BoltValue* value, struct double_triple x)
{
    _format(value, BOLT_FLOAT64_TRIPLE, 1, &x, sizeof(x));
}

void BoltValue_to_Float64Quad(struct BoltValue* value, struct double_quad x)
{
    _format(value, BOLT_FLOAT64_QUAD, 1, &x, sizeof(x));
}

void BoltValue_to_Float64Array(struct BoltValue* value, double * array, int32_t size)
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

void BoltValue_to_Float64PairArray(struct BoltValue* value, struct double_pair * array, int32_t size)
{
    _format(value, BOLT_FLOAT64_PAIR_ARRAY, size, array, sizeof_n(struct double_pair, size));
}

void BoltValue_to_Float64TripleArray(struct BoltValue* value, struct double_triple * array, int32_t size)
{
    _format(value, BOLT_FLOAT64_TRIPLE_ARRAY, size, array, sizeof_n(struct double_triple, size));
}

void BoltValue_to_Float64QuadArray(struct BoltValue* value, struct double_quad * array, int32_t size)
{
    _format(value, BOLT_FLOAT64_QUAD_ARRAY, size, array, sizeof_n(struct double_quad, size));
}

double BoltFloat64_get(const struct BoltValue* value)
{
    return value->data.as_double[0];
}

struct double_pair BoltFloat64Pair_get(const struct BoltValue* value)
{
    return value->data.as_double_pair[0];
}

struct double_triple BoltFloat64Triple_get(const struct BoltValue* value)
{
    return value->data.extended.as_double_triple[0];
}

struct double_quad BoltFloat64Quad_get(const struct BoltValue* value)
{
    return value->data.extended.as_double_quad[0];
}

double BoltFloat64Array_get(const struct BoltValue* value, int32_t index)
{
    const double* data = value->size <= sizeof(value->data) / sizeof(double) ?
                         value->data.as_double : value->data.extended.as_double;
    return data[index];
}

struct double_pair BoltFloat64PairArray_get(const struct BoltValue * value, int32_t index)
{
    return value->data.extended.as_double_pair[index];
}

struct double_triple BoltFloat64TripleArray_get(const struct BoltValue * value, int32_t index)
{
    return value->data.extended.as_double_triple[index];
}

struct double_quad BoltFloat64QuadArray_get(const struct BoltValue * value, int32_t index)
{
    return value->data.extended.as_double_quad[index];
}

int BoltFloat64_write(struct BoltValue * value, FILE * file)
{
    assert(BoltValue_type(value) == BOLT_FLOAT64);
    fprintf(file, "f64(%f)", BoltFloat64_get(value));
    return 0;
}

int BoltFloat64Pair_write(struct BoltValue * value, FILE * file)
{
    assert(BoltValue_type(value) == BOLT_FLOAT64_PAIR);
    struct double_pair tuple = BoltFloat64Pair_get(value);
    fprintf(file, "ff64(%f %f)", tuple.x, tuple.y);
    return 0;
}

int BoltFloat64Triple_write(struct BoltValue * value, FILE * file)
{
    assert(BoltValue_type(value) == BOLT_FLOAT64_TRIPLE);
    struct double_triple tuple = BoltFloat64Triple_get(value);
    fprintf(file, "fff64(%f %f %f)", tuple.x, tuple.y, tuple.z);
    return 0;
}

int BoltFloat64Quad_write(struct BoltValue * value, FILE * file)
{
    assert(BoltValue_type(value) == BOLT_FLOAT64_QUAD);
    struct double_quad tuple = BoltFloat64Quad_get(value);
    fprintf(file, "ffff64(%f %f %f %f)", tuple.x, tuple.y, tuple.z, tuple.a);
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

int BoltFloat64PairArray_write(struct BoltValue * value, FILE * file)
{
    assert(BoltValue_type(value) == BOLT_FLOAT64_PAIR_ARRAY);
    fprintf(file, "ff64[");
    for (int i = 0; i < value->size; i++)
    {
        struct double_pair tuple = BoltFloat64PairArray_get(value, i);
        fprintf(file, i == 0 ? "%f %f" : ", %f %f", tuple.x, tuple.y);
    }
    fprintf(file, "]");
    return 0;
}

int BoltFloat64TripleArray_write(struct BoltValue * value, FILE * file)
{
    assert(BoltValue_type(value) == BOLT_FLOAT64_TRIPLE_ARRAY);
    fprintf(file, "fff64[");
    for (int i = 0; i < value->size; i++)
    {
        struct double_triple tuple = BoltFloat64TripleArray_get(value, i);
        fprintf(file, i == 0 ? "%f %f %f" : ", %f %f %f", tuple.x, tuple.y, tuple.z);
    }
    fprintf(file, "]");
    return 0;
}

int BoltFloat64QuadArray_write(struct BoltValue * value, FILE * file)
{
    assert(BoltValue_type(value) == BOLT_FLOAT64_QUAD_ARRAY);
    fprintf(file, "ffff64[");
    for (int i = 0; i < value->size; i++)
    {
        struct double_quad tuple = BoltFloat64QuadArray_get(value, i);
        fprintf(file, i == 0 ? "%f %f %f %f" : ", %f %f %f %f", tuple.x, tuple.y, tuple.z, tuple.a);
    }
    fprintf(file, "]");
    return 0;
}
