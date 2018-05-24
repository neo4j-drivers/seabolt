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
#include <inttypes.h>
#include <memory.h>

#include "bolt/mem.h"
#include "bolt/values.h"

#include "protocol/v1.h"



#define STRING_QUOTE '"'
#define CODE_POINT_OPEN_BRACKET '{'
#define CODE_POINT_CLOSE_BRACKET '}'
#define REPLACEMENT_CHARACTER "\xFF\xFD"

#define IS_PRINTABLE_ASCII(ch) ((ch) >= ' ' && (ch) <= '~')



/**
 * Clean up a value for reuse.
 *
 * This sets any nested values to null.
 *
 * @param value
 */
void _recycle(struct BoltValue* value)
{
    enum BoltType type = BoltValue_type(value);
    if (type == BOLT_LIST || type == BOLT_STRUCTURE || type == BOLT_MESSAGE)
    {
        for (long i = 0; i < value->size; i++)
        {
            BoltValue_format_as_Null(&value->data.extended.as_value[i]);
        }
    }
    else if (type == BOLT_DICTIONARY)
    {
        for (long i = 0; i < 2 * value->size; i++)
        {
            BoltValue_format_as_Null(&value->data.extended.as_value[i]);
        }
    }
}

/**
 * Resize a value that contains multiple sub-values.
 *
 * @param value
 * @param size
 * @param multiplier
 */
void _resize(struct BoltValue* value, int32_t size, int multiplier)
{
    if (size > value->size)
    {
        // grow physically
        size_t unit_size = sizeof(struct BoltValue);
        size_t new_data_size = multiplier * unit_size * size;
        size_t old_data_size = value->data_size;
        value->data.extended.as_ptr = BoltMem_adjust(value->data.extended.as_ptr, value->data_size, new_data_size);
        value->data_size = new_data_size;
        // grow logically
        memset(value->data.extended.as_char + old_data_size, 0, new_data_size - old_data_size);
        value->size = size;
    }
    else if (size < value->size)
    {
        // shrink logically
        for (long i = multiplier * size; i < multiplier * value->size; i++)
        {
            BoltValue_format_as_Null(&value->data.extended.as_value[i]);
        }
        value->size = size;
        // shrink physically
        size_t new_data_size = multiplier * sizeof_n(struct BoltValue, size);
        value->data.extended.as_ptr = BoltMem_adjust(value->data.extended.as_ptr, value->data_size, new_data_size);
        value->data_size = new_data_size;
    }
    else
    {
        // same size - do nothing
    }
}

void _set_type(struct BoltValue* value, enum BoltType type, int16_t subtype, int32_t size)
{
    assert(type < 0x80);
    value->type = (char)(type);
    value->subtype = subtype;
    value->size = size;
}

void _format(struct BoltValue* value, enum BoltType type, int16_t subtype, int32_t size, const void* data, size_t data_size)
{
    _recycle(value);
    value->data.extended.as_ptr = BoltMem_adjust(value->data.extended.as_ptr, value->data_size, data_size);
    value->data_size = data_size;
    if (data != NULL && data_size > 0)
    {
        memcpy(value->data.extended.as_char, data, data_size);
    }
    _set_type(value, type, subtype, size);
}

void _format_as_structure(struct BoltValue * value, enum BoltType type, int16_t code, int32_t size)
{
    _recycle(value);
    value->data.extended.as_ptr = BoltMem_adjust(value->data.extended.as_ptr, value->data_size,
                                                 sizeof_n(struct BoltValue, size));
    value->data_size = sizeof_n(struct BoltValue, size);
    memset(value->data.extended.as_char, 0, value->data_size);
    _set_type(value, type, code, size);
}

void _write_string(FILE * file, const char * data, size_t size)
{
    fputc(STRING_QUOTE, file);
    for (size_t i = 0; i < size; i++)
    {
        fputc(data[i], file);
    }
    fputc(STRING_QUOTE, file);
}

void _write_code_point(FILE * file, const uint32_t ch)
{
    if (ch < 0x10000)
    {
        fprintf(file, "U+%c%c%c%c", hex3(&ch, 0), hex2(&ch, 0), hex1(&ch, 0), hex0(&ch, 0));
    }
    else if (ch < 0x100000)
    {
        fprintf(file, "U+%c%c%c%c%c", hex4(&ch, 0), hex3(&ch, 0), hex2(&ch, 0), hex1(&ch, 0), hex0(&ch, 0));
    }
    else if (ch < 0x1000000)
    {
        fprintf(file, "U+%c%c%c%c%c%c", hex5(&ch, 0), hex4(&ch, 0), hex3(&ch, 0), hex2(&ch, 0), hex1(&ch, 0), hex0(&ch, 0));
    }
    else
    {
        fprintf(file, REPLACEMENT_CHARACTER);
    }
}

void _write_bracketed_code_point(FILE * file, const uint32_t ch)
{
    fputc(CODE_POINT_OPEN_BRACKET, file);
    _write_code_point(file, ch);
    fputc(CODE_POINT_CLOSE_BRACKET, file);
}



struct BoltValue* BoltValue_create()
{
    size_t size = sizeof(struct BoltValue);
    struct BoltValue* value = BoltMem_allocate(size);
    _set_type(value, BOLT_NULL, 0, 0);
    value->data_size = 0;
    value->data.as_int64[0] = 0;
    value->data.as_int64[1] = 0;
    value->data.extended.as_ptr = NULL;
    return value;
}

void BoltValue_destroy(struct BoltValue* value)
{
    BoltValue_format_as_Null(value);
    BoltMem_deallocate(value, sizeof(struct BoltValue));
}

enum BoltType BoltValue_type(const struct BoltValue* value)
{
    return (enum BoltType)(value->type);
}

int BoltValue_write(struct BoltValue * value, FILE * file, int32_t protocol_version)
{
    switch (BoltValue_type(value))
    {
        case BOLT_NULL:
            return BoltNull_write(value, file);
        case BOLT_BOOLEAN:
            return BoltBoolean_write(value, file);
        case BOLT_INTEGER:
            return BoltInteger_write(value, file);
        case BOLT_FLOAT:
            return BoltFloat_write(value, file);
        case BOLT_STRING:
            return BoltString_write(value, file);
        case BOLT_DICTIONARY:
            return BoltDictionary_write(value, file, protocol_version);
        case BOLT_LIST:
            return BoltList_write(value, file, protocol_version);
        case BOLT_BYTES:
            return BoltBytes_write(value, file);
        case BOLT_STRUCTURE:
            return BoltStructure_write(value, file, protocol_version);
        case BOLT_MESSAGE:
            return BoltMessage_write(value, file, protocol_version);
        default:
            fprintf(file, "?");
            return -1;
    }
}



void BoltValue_format_as_Null(struct BoltValue * value)
{
    if (BoltValue_type(value) != BOLT_NULL)
    {
        _format(value, BOLT_NULL, 0, 0, NULL, 0);
    }
}

int BoltNull_write(const struct BoltValue * value, FILE * file)
{
    assert(BoltValue_type(value) == BOLT_NULL);
    fprintf(file, "null");
    return 0;
}



void BoltValue_format_as_Boolean(struct BoltValue * value, char data)
{
    _format(value, BOLT_BOOLEAN, 0, 1, NULL, 0);
    value->data.as_char[0] = data;
}

char BoltBoolean_get(const struct BoltValue * value)
{
    return to_bit(value->data.as_char[0]);
}

int BoltBoolean_write(const struct BoltValue * value, FILE * file)
{
    assert(BoltValue_type(value) == BOLT_BOOLEAN);
    fprintf(file, "!%d", BoltBoolean_get(value));
    return 0;
}



void BoltValue_format_as_Integer(struct BoltValue * value, int64_t data)
{
    _format(value, BOLT_INTEGER, 0, 1, NULL, 0);
    value->data.as_int64[0] = data;
}

int64_t BoltInteger_get(const struct BoltValue * value)
{
    return value->data.as_int64[0];
}

int BoltInteger_write(struct BoltValue * value, FILE * file)
{
    assert(BoltValue_type(value) == BOLT_INTEGER);
    fprintf(file, "%" PRIi64, BoltInteger_get(value));
    return 0;
}



void BoltValue_format_as_Float(struct BoltValue * value, double data)
{
    _format(value, BOLT_FLOAT, 0, 1, NULL, 0);
    value->data.as_double[0] = data;
}

double BoltFloat_get(const struct BoltValue * value)
{
    return value->data.as_double[0];
}

int BoltFloat_write(struct BoltValue * value, FILE * file)
{
    assert(BoltValue_type(value) == BOLT_FLOAT);
    fprintf(file, "%f", BoltFloat_get(value));
    return 0;
}



void BoltValue_format_as_String(struct BoltValue * value, const char * data, int32_t length)
{
    size_t data_size = length >= 0 ? sizeof(char) * length : 0;
    if (length <= sizeof(value->data) / sizeof(char))
    {
        // If the string is short, it can fit entirely within the
        // BoltValue instance
        _format(value, BOLT_STRING, 0, length, NULL, 0);
        if (data != NULL)
        {
            memcpy(value->data.as_char, data, (size_t)(length));
        }
    }
    else if (BoltValue_type(value) == BOLT_STRING)
    {
        // This is already a UTF-8 string so we can just tweak the value
        value->data.extended.as_ptr = BoltMem_adjust(value->data.extended.as_ptr, value->data_size, data_size);
        value->data_size = data_size;
        value->size = length;
        if (data != NULL)
        {
            memcpy(value->data.extended.as_char, data, data_size);
        }
    }
    else
    {
        // If all else fails, allocate some new external data space
        _format(value, BOLT_STRING, 0, length, data, data_size);
    }
}

char * BoltString_get(struct BoltValue * value)
{
    return value->size <= sizeof(value->data) / sizeof(char) ?
           value->data.as_char : value->data.extended.as_char;
}

int BoltString_write(struct BoltValue * value, FILE * file)
{
    assert(BoltValue_type(value) == BOLT_STRING);
    char * data = BoltString_get(value);
    fputc(STRING_QUOTE, file);
    for (int i = 0; i < value->size; i++)
    {
        char ch0 = data[i];
        if (IS_PRINTABLE_ASCII(ch0) && ch0 != STRING_QUOTE &&
            ch0 != CODE_POINT_OPEN_BRACKET && ch0 != CODE_POINT_CLOSE_BRACKET)
        {
            fputc(ch0, file);
        }
        else if (ch0 >= 0)
        {
            _write_bracketed_code_point(file, (uint32_t)(ch0));
        }
        else if ((ch0 & 0b11100000) == 0b11000000)
        {
            if (i + 1 < value->size)
            {
                char ch1 = data[++i];
                uint32_t ch = ((ch0 & (uint32_t)(0b00011111)) << 6) | (ch1 & (uint32_t)(0b00111111));
                _write_bracketed_code_point(file, ch);
            }
            else
            {
                fprintf(file, REPLACEMENT_CHARACTER);
            }
        }
        else if ((ch0 & 0b11110000) == 0b11100000)
        {
            if (i + 2 < value->size)
            {
                char ch1 = data[++i];
                char ch2 = data[++i];
                uint32_t ch = ((ch0 & (uint32_t)(0b00001111)) << 12) | ((ch1 & (uint32_t)(0b00111111)) << 6) | (ch2 & (uint32_t)(0b00111111));
                _write_bracketed_code_point(file, ch);
            }
            else
            {
                fprintf(file, REPLACEMENT_CHARACTER);
            }
        }
        else if ((ch0 & 0b11111000) == 0b11110000)
        {
            if (i + 3 < value->size)
            {
                char ch1 = data[++i];
                char ch2 = data[++i];
                char ch3 = data[++i];
                uint32_t ch = ((ch0 & (uint32_t)(0b00000111)) << 18) | ((ch1 & (uint32_t)(0b00111111)) << 12) | ((ch2 & (uint32_t)(0b00111111)) << 6) | (ch3 & (uint32_t)(0b00111111));
                _write_bracketed_code_point(file, ch);
            }
            else
            {
                fprintf(file, REPLACEMENT_CHARACTER);
            }
        }
        else
        {
            fprintf(file, REPLACEMENT_CHARACTER);
        }
    }
    fputc(STRING_QUOTE, file);
    return 0;
}



void BoltValue_format_as_Dictionary(struct BoltValue * value, int32_t length)
{
    if (value->type == BOLT_DICTIONARY)
    {
        _resize(value, length, 2);
    }
    else
    {
        size_t unit_size = sizeof(struct BoltValue);
        size_t data_size = 2 * unit_size * length;
        _recycle(value);
        value->data.extended.as_ptr = BoltMem_adjust(value->data.extended.as_ptr, value->data_size, data_size);
        value->data_size = data_size;
        memset(value->data.extended.as_char, 0, data_size);
        _set_type(value, BOLT_DICTIONARY, 0, length);
    }
}

struct BoltValue* BoltDictionary_key(struct BoltValue * value, int32_t index)
{
    assert(BoltValue_type(value) == BOLT_DICTIONARY);
    return &value->data.extended.as_value[2 * index];
}

const char * BoltDictionary_get_key(struct BoltValue * value, int32_t index)
{
    assert(BoltValue_type(value) == BOLT_DICTIONARY);
    struct BoltValue * key_value = &value->data.extended.as_value[2 * index];
    assert(BoltValue_type(key_value) == BOLT_STRING);
    return BoltString_get(key_value);
}

int32_t BoltDictionary_get_key_size(struct BoltValue * value, int32_t index)
{
    assert(BoltValue_type(value) == BOLT_DICTIONARY);
    struct BoltValue * key_value = &value->data.extended.as_value[2 * index];
    assert(BoltValue_type(key_value) == BOLT_STRING);
    return key_value->size;
}

int BoltDictionary_set_key(struct BoltValue * value, int32_t index, const char * key, size_t key_size)
{
    if (key_size <= INT32_MAX)
    {
        assert(BoltValue_type(value) == BOLT_DICTIONARY);
        BoltValue_format_as_String(&value->data.extended.as_value[2 * index], key, (int32_t)key_size);
        return 0;
    }
    else
    {
        return -1;
    }
}

struct BoltValue* BoltDictionary_value(struct BoltValue * value, int32_t index)
{
    assert(BoltValue_type(value) == BOLT_DICTIONARY);
    return &value->data.extended.as_value[2 * index + 1];
}

int BoltDictionary_write(struct BoltValue * value, FILE * file, int32_t protocol_version)
{
    assert(BoltValue_type(value) == BOLT_DICTIONARY);
    fprintf(file, "{");
    int comma = 0;
    for (int i = 0; i < value->size; i++)
    {
        const char * key = BoltDictionary_get_key(value, i);
        if (key != NULL)
        {
            if (comma) fprintf(file, ", ");
            _write_string(file, key, (size_t)(BoltDictionary_get_key_size(value, i)));
            fprintf(file, ": ");
            BoltValue_write(BoltDictionary_value(value, i), file, protocol_version);
            comma = 1;
        }
    }
    fprintf(file, "}");
    return 0;
}



void BoltValue_format_as_List(struct BoltValue * value, int32_t length)
{
    if (BoltValue_type(value) == BOLT_LIST)
    {
        BoltList_resize(value, length);
    }
    else
    {
        size_t data_size = sizeof(struct BoltValue) * length;
        _recycle(value);
        value->data.extended.as_ptr = BoltMem_adjust(value->data.extended.as_ptr, value->data_size, data_size);
        value->data_size = data_size;
        memset(value->data.extended.as_char, 0, data_size);
        _set_type(value, BOLT_LIST, 0, length);
    }
}

void BoltList_resize(struct BoltValue* value, int32_t size)
{
    assert(BoltValue_type(value) == BOLT_LIST);
    _resize(value, size, 1);
}

struct BoltValue* BoltList_value(const struct BoltValue* value, int32_t index)
{
    assert(BoltValue_type(value) == BOLT_LIST);
    return &value->data.extended.as_value[index];
}

int BoltList_write(const struct BoltValue * value, FILE * file, int32_t protocol_version)
{
    assert(BoltValue_type(value) == BOLT_LIST);
    fprintf(file, "[");
    for (int i = 0; i < value->size; i++)
    {
        if (i > 0) fprintf(file, ", ");
        BoltValue_write(BoltList_value(value, i), file, protocol_version);
    }
    fprintf(file, "]");
    return 0;
}



void BoltValue_format_as_Bytes(struct BoltValue * value, char * data, int32_t length)
{
    if (length <= sizeof(value->data) / sizeof(char))
    {
        _format(value, BOLT_BYTES, 0, length, NULL, 0);
        if (data != NULL)
        {
            memcpy(value->data.as_char, data, (size_t)(length));
        }
    }
    else
    {
        _format(value, BOLT_BYTES, 0, length, data, sizeof_n(char, length));
    }
}

char BoltBytes_get(const struct BoltValue * value, int32_t index)
{
    const char* data = value->size <= sizeof(value->data) / sizeof(char) ?
                       value->data.as_char : value->data.extended.as_char;
    return data[index];
}

char* BoltBytes_get_all(struct BoltValue * value)
{
    return value->size <= sizeof(value->data) / sizeof(char) ?
           value->data.as_char : value->data.extended.as_char;
}

int BoltBytes_write(const struct BoltValue * value, FILE * file)
{
    assert(BoltValue_type(value) == BOLT_BYTES);
    fprintf(file, "#[");
    for (int i = 0; i < value->size; i++)
    {
        char b = BoltBytes_get(value, i);
        fprintf(file, "%c%c", hex1(&b, 0), hex0(&b, 0));
    }
    fprintf(file, "]");
    return 0;
}

void BoltValue_format_as_Structure(struct BoltValue * value, int16_t code, int32_t length)
{
    _format_as_structure(value, BOLT_STRUCTURE, code, length);
}

int16_t BoltStructure_code(const struct BoltValue* value)
{
    assert(BoltValue_type(value) == BOLT_STRUCTURE);
    return value->subtype;
}

struct BoltValue* BoltStructure_value(const struct BoltValue* value, int32_t index)
{
    assert(BoltValue_type(value) == BOLT_STRUCTURE);
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



void BoltValue_format_as_Message(struct BoltValue * value, int16_t code, int32_t length)
{
    _format_as_structure(value, BOLT_MESSAGE, code, length);
}

int16_t BoltMessage_code(const struct BoltValue * value)
{
    assert(BoltValue_type(value) == BOLT_MESSAGE);
    return value->subtype;
}

struct BoltValue* BoltMessage_value(const struct BoltValue * value, int32_t index)
{
    assert(BoltValue_type(value) == BOLT_MESSAGE);
    return &value->data.extended.as_value[index];
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
