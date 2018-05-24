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

#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <bolt/values.h>
#include "../protocol/v1.h"
#include "bolt/mem.h"



void _to_structure(struct BoltValue* value, enum BoltType type, int16_t code, int32_t size)
{
    _recycle(value);
    value->data.extended.as_ptr = BoltMem_adjust(value->data.extended.as_ptr, value->data_size,
                                                 sizeof_n(struct BoltValue, size));
    value->data_size = sizeof_n(struct BoltValue, size);
    memset(value->data.extended.as_char, 0, value->data_size);
    _set_type(value, type, code, size);
}

void BoltValue_format_as_Structure(struct BoltValue * value, int16_t code, int32_t length)
{
    _to_structure(value, BOLT_STRUCTURE, code, length);
}

void BoltValue_format_as_Message(struct BoltValue * value, int16_t code, int32_t length)
{
    _to_structure(value, BOLT_MESSAGE, code, length);
}

int16_t BoltStructure_code(const struct BoltValue* value)
{
    assert(BoltValue_type(value) == BOLT_STRUCTURE);
    return value->subtype;
}

int16_t BoltMessage_code(const struct BoltValue * value)
{
    assert(BoltValue_type(value) == BOLT_MESSAGE);
    return value->subtype;
}

struct BoltValue* BoltStructure_value(const struct BoltValue* value, int32_t index)
{
    assert(BoltValue_type(value) == BOLT_STRUCTURE);
    return &value->data.extended.as_value[index];
}

struct BoltValue* BoltMessage_value(const struct BoltValue * value, int32_t index)
{
    assert(BoltValue_type(value) == BOLT_MESSAGE);
    return &value->data.extended.as_value[index];
}

int BoltStructure_write(struct BoltValue * value, FILE * file, int32_t protocol_version)
{
    assert(BoltValue_type(value) == BOLT_STRUCTURE);
    int16_t code = BoltStructure_code(value);
    switch (protocol_version)
    {
        case 1:
        {
            const char* name = BoltProtocolV1_structure_name(code);
            fprintf(file, "&%s", name);
            break;
        }
        default:
            fprintf(file, "&#%c%c%c%c", hex3(&code, 0), hex2(&code, 0), hex1(&code, 0), hex0(&code, 0));
    }
    fprintf(file, "(");
    for (int i = 0; i < value->size; i++)
    {
        if (i > 0) fprintf(file, " ");
        BoltValue_write(BoltStructure_value(value, i), file, protocol_version);
    }
    fprintf(file, ")");
    return 0;
}

int BoltMessage_write(struct BoltValue * value, FILE * file, int32_t protocol_version)
{
    assert(BoltValue_type(value) == BOLT_MESSAGE);
    int16_t code = BoltMessage_code(value);
    switch (protocol_version)
    {
        case 1:
        {
            const char* name = BoltProtocolV1_message_name(code);
            if (name == NULL)
            {
                fprintf(file, "msg<#%c%c>", hex1(&code, 0), hex0(&code, 0));
            }
            else
            {
                fprintf(file, "%s", name);
            }
            break;
        }
        default:
            fprintf(file, "msg<#%c%c>", hex1(&code, 0), hex0(&code, 0));
    }
    for (int i = 0; i < value->size; i++)
    {
        fprintf(file, " ");
        BoltValue_write(BoltMessage_value(value, i), file, protocol_version);
    }
    return 0;
}
