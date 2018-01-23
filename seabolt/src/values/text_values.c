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
#include "mem.h"


static const char * const REPLACEMENT_CHARACTER = "\xFF\xFD";


void BoltValue_to_Char(struct BoltValue * value, uint32_t x)
{
    _format(value, BOLT_CHAR, 1, NULL, 0);
    value->data.as_uint32[0] = x;
}

uint32_t BoltChar_get(const struct BoltValue * value)
{
    return value->data.as_uint32[0];
}

void BoltValue_to_String(struct BoltValue * value, const char * string, int32_t size)
{
    size_t data_size = size >= 0 ? sizeof(char) * size : 0;
    if (size <= sizeof(value->data) / sizeof(char))
    {
        // If the string is short, it can fit entirely within the
        // BoltValue instance
        _format(value, BOLT_STRING, size, NULL, 0);
        if (string != NULL)
        {
            memcpy(value->data.as_char, string, (size_t)(size));
        }
    }
    else if (BoltValue_type(value) == BOLT_STRING)
    {
        // This is already a UTF-8 string so we can just tweak the value
        value->data.extended.as_ptr = BoltMem_adjust(value->data.extended.as_ptr, value->data_size, data_size);
        value->data_size = data_size;
        value->size = size;
        if (string != NULL)
        {
            memcpy(value->data.extended.as_char, string, data_size);
        }
    }
    else
    {
        // If all else fails, allocate some new external data space
        _format(value, BOLT_STRING, size, string, data_size);
    }
}

void BoltValue_to_CharArray(struct BoltValue * value, const uint32_t * array, int32_t size)
{
    size_t data_size = size >= 0 ? sizeof(uint32_t) * size : 0;
    if (size <= sizeof(value->data) / sizeof(uint32_t))
    {
        _format(value, BOLT_CHAR_ARRAY, size, NULL, 0);
        if (array != NULL)
        {
            memcpy(value->data.as_uint32, array, data_size);
        }
    }
    else if (BoltValue_type(value) == BOLT_CHAR_ARRAY)
    {
        value->data.extended.as_ptr = BoltMem_adjust(value->data.extended.as_ptr, value->data_size, data_size);
        value->data_size = data_size;
        value->size = size;
        if (array != NULL)
        {
            memcpy(value->data.extended.as_uint32, array, data_size);
        }
    }
    else
    {
        // If all else fails, allocate some new external data space
        _format(value, BOLT_CHAR_ARRAY, size, array, data_size);
    }
}

void BoltValue_to_StringArray(struct BoltValue * value, int32_t size)
{
    _format(value, BOLT_STRING_ARRAY, size, NULL, sizeof_n(struct array_t, size));
    for (int i = 0; i < size; i++)
    {
        value->data.extended.as_array[i].size = 0;
        value->data.extended.as_array[i].data.as_ptr = NULL;
    }
}

void BoltValue_to_Dictionary(struct BoltValue * value, int32_t size)
{
    if (value->type == BOLT_DICTIONARY)
    {
        _resize(value, size, 2);
    }
    else
    {
        size_t unit_size = sizeof(struct BoltValue);
        size_t data_size = 2 * unit_size * size;
        _recycle(value);
        value->data.extended.as_ptr = BoltMem_adjust(value->data.extended.as_ptr, value->data_size, data_size);
        value->data_size = data_size;
        memset(value->data.extended.as_char, 0, data_size);
        _set_type(value, BOLT_DICTIONARY, size);
    }
}

char * BoltString_get(struct BoltValue * value)
{
    return value->size <= sizeof(value->data) / sizeof(char) ?
           value->data.as_char : value->data.extended.as_char;
}

uint32_t * BoltCharArray_get(struct BoltValue * value)
{
    return value->size <= sizeof(value->data) / sizeof(uint32_t) ?
           value->data.as_uint32 : value->data.extended.as_uint32;
}

char* BoltStringArray_get(struct BoltValue * value, int32_t index)
{
    struct array_t string = value->data.extended.as_array[index];
    return string.size == 0 ? NULL : string.data.as_char;
}

int32_t BoltStringArray_get_size(struct BoltValue * value, int32_t index)
{
    return value->data.extended.as_array[index].size;
}

void BoltStringArray_put(struct BoltValue * value, int32_t index, const char * string, int32_t size)
{
    struct array_t* s = &value->data.extended.as_array[index];
    s->data.as_ptr = BoltMem_adjust(s->data.as_ptr, (size_t)(s->size), (size_t)(size));
    s->size = size;
    if (size > 0)
    {
        memcpy(s->data.as_ptr, string, (size_t)(size));
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
        BoltValue_to_String(&value->data.extended.as_value[2 * index], key, key_size);
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

void _write_string(FILE* file, const char* data, size_t size)
{
    fprintf(file, "\"");
    for (size_t i = 0; i < size; i++)
    {
        fprintf(file, "%c", data[i]);
    }
    fprintf(file, "\"");
}

int _write_char(FILE* file, const uint32_t ch)
{
    if (ch >= 0x20 && ch <= 0x7E && ch != '\'')
    {
        fprintf(file, "'%c'", (char)(ch));
        return 0;
    }
    if (ch < 0x10000)
    {
        fprintf(file, "U+%c%c%c%c", hex3(&ch, 0), hex2(&ch, 0), hex1(&ch, 0), hex0(&ch, 0));
        return 0;
    }
    if (ch < 0x100000)
    {
        fprintf(file, "U+%c%c%c%c%c", hex4(&ch, 0), hex3(&ch, 0), hex2(&ch, 0), hex1(&ch, 0), hex0(&ch, 0));
        return 0;
    }
    if (ch < 0x1000000)
    {
        fprintf(file, "U+%c%c%c%c%c%c", hex5(&ch, 0), hex4(&ch, 0), hex3(&ch, 0), hex2(&ch, 0), hex1(&ch, 0), hex0(&ch, 0));
        return 0;
    }
    fprintf(file, "?");
    return -1;
}

int BoltChar_write(const struct BoltValue * value, FILE * file)
{
    assert(BoltValue_type(value) == BOLT_CHAR);
    fprintf(file, "char(");
    int status = _write_char(file, BoltChar_get(value));
    fprintf(file, ")");
    return status;
}

int BoltCharArray_write(struct BoltValue * value, FILE * file)
{
    assert(BoltValue_type(value) == BOLT_CHAR_ARRAY);
    uint32_t * array = BoltCharArray_get(value);
    fprintf(file, "char[");
    for (int32_t i = 0; i < value->size; i++)
    {
        if (i > 0) { fprintf(file, ", "); }
        _write_char(file, array[i]);
    }
    fprintf(file, "]");
    return 0;
}

int BoltString_write(struct BoltValue * value, FILE * file)
{
    assert(BoltValue_type(value) == BOLT_STRING);
    char * data = BoltString_get(value);
    fprintf(file, "str(\"");
    for (int i = 0; i < value->size; i++)
    {
        char ch0 = data[i];
        if (ch0 >= ' ' && ch0 <= '~' && ch0 != '"')
        {
            fprintf(file, "%c", ch0);
        }
        else if (ch0 >= 0)
        {
            fprintf(file, "\\u00%c%c", hex1(&ch0, 0), hex0(&ch0, 0));
        }
        else if ((ch0 & 0b11100000) == 0b11000000)
        {
            if (i + 1 < value->size)
            {
                char ch1 = data[++i];
                uint32_t ch = ((ch0 & (uint32_t)(0b00011111)) << 6) | (ch1 & (uint32_t)(0b00111111));
                fprintf(file, "\\u%c%c%c%c", hex3(&ch, 0), hex2(&ch, 0), hex1(&ch, 0), hex0(&ch, 0));
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
                fprintf(file, "\\u%c%c%c%c", hex3(&ch, 0), hex2(&ch, 0), hex1(&ch, 0), hex0(&ch, 0));
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
                fprintf(file, "\\U%c%c%c%c%c%c%c%c", hex7(&ch, 0), hex6(&ch, 0), hex5(&ch, 0), hex4(&ch, 0), hex3(&ch, 0), hex2(&ch, 0), hex1(&ch, 0), hex0(&ch, 0));
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
    fprintf(file, "\")");
    return 0;
}

int BoltStringArray_write(struct BoltValue * value, FILE * file)
{
    assert(BoltValue_type(value) == BOLT_STRING_ARRAY);
    fprintf(file, "str[");
    for (long i = 0; i < value->size; i++)
    {
        if (i > 0) { fprintf(file, ", "); }
        struct array_t string = value->data.extended.as_array[i];
        if (string.size == 0)
        {
            fprintf(file, "\"\"");
        }
        else
        {
            _write_string(file, string.data.as_char, (size_t)(string.size));
        }
    }
    fprintf(file, "]");
    return 0;
}

int BoltDictionary_write(struct BoltValue * value, FILE * file, int32_t protocol_version)
{
    assert(BoltValue_type(value) == BOLT_DICTIONARY);
    fprintf(file, "dict[");
    int comma = 0;
    for (int i = 0; i < value->size; i++)
    {
        const char * key = BoltDictionary_get_key(value, i);
        if (key != NULL)
        {
            if (comma) fprintf(file, ", ");
            _write_string(file, key, (size_t)(BoltDictionary_get_key_size(value, i)));
            fprintf(file, " ");
            BoltValue_write(BoltDictionary_value(value, i), file, protocol_version);
            comma = 1;
        }
    }
    fprintf(file, "]");
    return 0;
}
