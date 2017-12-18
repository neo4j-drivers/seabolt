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
#include <string.h>
#include <values.h>
#include <protocol_v1.h>


static const char HEX_DIGITS[] = {'0', '1', '2', '3', '4', '5', '6', '7',
                                  '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

#define hex3(mem, offset) HEX_DIGITS[((mem)[offset] >> 12) & 0x0F]

#define hex2(mem, offset) HEX_DIGITS[((mem)[offset] >> 8) & 0x0F]

#define hex1(mem, offset) HEX_DIGITS[((mem)[offset] >> 4) & 0x0F]

#define hex0(mem, offset) HEX_DIGITS[(mem)[offset] & 0x0F]


void _to_structure(struct BoltValue* value, enum BoltType type, int16_t code, int32_t size)
{
    _recycle(value);
    value->data.extended.as_ptr = BoltMem_adjust(value->data.extended.as_ptr, value->data_size,
                                                 sizeof_n(struct BoltValue, size));
    value->data_size = sizeof_n(struct BoltValue, size);
    memset(value->data.extended.as_char, 0, value->data_size);
    _set_type(value, type, size);
    value->code = code;
}

void BoltValue_to_Structure(struct BoltValue* value, int16_t code, int32_t size)
{
    _to_structure(value, BOLT_STRUCTURE, code, size);
}

void BoltValue_to_StructureArray(struct BoltValue* value, int16_t code, int32_t size)
{
    _to_structure(value, BOLT_STRUCTURE_ARRAY, code, size);
    for (long i = 0; i < size; i++)
    {
        BoltValue_to_List(&value->data.extended.as_value[i], 0);
    }
}

void BoltValue_to_Request(struct BoltValue* value, int16_t code, int32_t size)
{
    _to_structure(value, BOLT_REQUEST, code, size);
}

void BoltValue_to_Summary(struct BoltValue* value, int16_t code, int32_t size)
{
    _to_structure(value, BOLT_SUMMARY, code, size);
}

int16_t BoltStructure_code(const struct BoltValue* value)
{
    assert(BoltValue_type(value) == BOLT_STRUCTURE || BoltValue_type(value) == BOLT_STRUCTURE_ARRAY);
    return value->code;
}

int16_t BoltRequest_code(const struct BoltValue* value)
{
    assert(BoltValue_type(value) == BOLT_REQUEST);
    return value->code;
}

int16_t BoltSummary_code(const struct BoltValue* value)
{
    assert(BoltValue_type(value) == BOLT_SUMMARY);
    return value->code;
}

struct BoltValue* BoltStructure_value(const struct BoltValue* value, int32_t index)
{
    assert(BoltValue_type(value) == BOLT_STRUCTURE);
    return &value->data.extended.as_value[index];
}

struct BoltValue* BoltRequest_value(const struct BoltValue* value, int32_t index)
{
    assert(BoltValue_type(value) == BOLT_REQUEST);
    return &value->data.extended.as_value[index];
}

struct BoltValue* BoltSummary_value(const struct BoltValue* value, int32_t index)
{
    assert(BoltValue_type(value) == BOLT_SUMMARY);
    return &value->data.extended.as_value[index];
}

int32_t BoltStructureArray_get_size(const struct BoltValue* value, int32_t index)
{
    return value->data.extended.as_value[index].size;
}

void BoltStructureArray_set_size(struct BoltValue* value, int32_t index, int32_t size)
{
    BoltList_resize(&value->data.extended.as_value[index], size);
}

struct BoltValue* BoltStructureArray_at(const struct BoltValue* value, int32_t array_index, int32_t structure_index)
{
    return BoltList_value(&value->data.extended.as_value[array_index], structure_index);
}

int BoltStructure_write(FILE* file, struct BoltValue* value, int32_t protocol_version)
{
    assert(BoltValue_type(value) == BOLT_STRUCTURE);
    int16_t code = BoltStructure_code(value);
    switch (protocol_version)
    {
        case 1:
        {
            const char* name = BoltProtocolV1_structure_name(code);
            fprintf(file, "$%s", name);
            break;
        }
        default:
            fprintf(file, "$#%c%c%c%c", hex3(&code, 0), hex2(&code, 0), hex1(&code, 0), hex0(&code, 0));
    }
    fprintf(file, "(");
    for (int i = 0; i < value->size; i++)
    {
        if (i > 0) fprintf(file, " ");
        BoltValue_write(file, BoltStructure_value(value, i), protocol_version);
    }
    fprintf(file, ")");
}

int BoltStructureArray_write(FILE* file, struct BoltValue* value, int32_t protocol_version)
{
    assert(BoltValue_type(value) == BOLT_STRUCTURE_ARRAY);
    int16_t code = BoltStructure_code(value);
    switch (protocol_version)
    {
        case 1:
        {
            const char* name = BoltProtocolV1_structure_name(code);
            if (name == NULL)
            {
                fprintf(file, "$#%c%c%c%c", hex3(&code, 0), hex2(&code, 0), hex1(&code, 0), hex0(&code, 0));
            }
            else
            {
                fprintf(file, "$%s", name);
            }
            break;
        }
        default:
            fprintf(file, "$#%c%c%c%c", hex3(&code, 0), hex2(&code, 0), hex1(&code, 0), hex0(&code, 0));
    }
    fprintf(file, "[");
    for (int i = 0; i < value->size; i++)
    {
        if (i > 0) fprintf(file, ", ");
        for (int j = 0; j < BoltStructureArray_get_size(value, i); j++)
        {
            if (j > 0) fprintf(file, " ");
            BoltValue_write(file, BoltStructureArray_at(value, i, j), protocol_version);
        }
    }
    fprintf(file, "]");
}

int BoltRequest_write(FILE* file, struct BoltValue* value, int32_t protocol_version)
{
    assert(BoltValue_type(value) == BOLT_REQUEST);
    int16_t code = BoltRequest_code(value);
    switch (protocol_version)
    {
        case 1:
        {
            const char* name = BoltProtocolV1_request_name(code);
            if (name == NULL)
            {
                fprintf(file, "Request<#%c%c>", hex1(&code, 0), hex0(&code, 0));
            }
            else
            {
                fprintf(file, "%s", name);
            }
            break;
        }
        default:
            fprintf(file, "Request<#%c%c>", hex1(&code, 0), hex0(&code, 0));
    }
    fprintf(file, "(");
    for (int i = 0; i < value->size; i++)
    {
        if (i > 0) fprintf(file, " ");
        BoltValue_write(file, BoltRequest_value(value, i), protocol_version);
    }
    fprintf(file, ")");
}

int BoltSummary_write(FILE* file, struct BoltValue* value, int32_t protocol_version)
{
    assert(BoltValue_type(value) == BOLT_SUMMARY);
    int16_t code = BoltSummary_code(value);
    switch (protocol_version)
    {
        case 1:
        {
            const char* name = BoltProtocolV1_summary_name(code);
            if (name == NULL)
            {
                fprintf(file, "Summary<#%c%c>", hex1(&code, 0), hex0(&code, 0));
            }
            else
            {
                fprintf(file, "%s", name);
            }
            break;
        }
        default:
            fprintf(file, "Summary<#%c%c>", hex1(&code, 0), hex0(&code, 0));
    }
    fprintf(file, "(");
    for (int i = 0; i < value->size; i++)
    {
        if (i > 0) fprintf(file, " ");
        BoltValue_write(file, BoltSummary_value(value, i), protocol_version);
    }
    fprintf(file, ")");
}
