/*
 * Copyright (c) 2002-2018 "Neo Technology,"
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

#include <string.h>
#include <bolt/values.h>
#include <assert.h>


void BoltValue_to_Float64(struct BoltValue* value, double data)
{
    _format(value, BOLT_FLOAT64, 0, 1, NULL, 0);
    value->data.as_double[0] = data;
}

void BoltValue_to_Float64Array(struct BoltValue* value, double * data, int32_t length)
{
    if (length <= sizeof(value->data) / sizeof(double))
    {
        _format(value, BOLT_FLOAT64_ARRAY, 0, length, NULL, 0);
        memcpy(value->data.as_double, data, (size_t)(length));
    }
    else
    {
        _format(value, BOLT_FLOAT64_ARRAY, 0, length, data, sizeof_n(double, length));
    }
}

double BoltFloat64_get(const struct BoltValue* value)
{
    return value->data.as_double[0];
}

double BoltFloat64Array_get(const struct BoltValue* value, int32_t index)
{
    const double* data = value->size <= sizeof(value->data) / sizeof(double) ?
                         value->data.as_double : value->data.extended.as_double;
    return data[index];
}

int BoltFloat64_write(struct BoltValue * value, FILE * file)
{
    assert(BoltValue_type(value) == BOLT_FLOAT64);
    fprintf(file, "%f", BoltFloat64_get(value));
    return 0;
}

int BoltFloat64Array_write(struct BoltValue * value, FILE * file)
{
    assert(BoltValue_type(value) == BOLT_FLOAT64_ARRAY);
    fprintf(file, ".[");
    for (int i = 0; i < value->size; i++)
    {
        if (i > 0) { fprintf(file, ", "); }
        fprintf(file, "%f", BoltFloat64_get(value));
    }
    fprintf(file, "]");
    return 0;
}
