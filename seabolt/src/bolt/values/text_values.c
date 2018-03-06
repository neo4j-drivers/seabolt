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

#include "bolt/mem.h"
#include "bolt/values.h"


#define CHAR_ARRAY_QUOTE  '\''
#define STRING_QUOTE '"'
#define CODE_POINT_OPEN_BRACKET '{'
#define CODE_POINT_CLOSE_BRACKET '}'
#define REPLACEMENT_CHARACTER "\xFF\xFD"

#define IS_PRINTABLE_ASCII(ch) ((ch) >= ' ' && (ch) <= '~')


void write_string(FILE * file, const char * data, size_t size)
{
    fputc(STRING_QUOTE, file);
    for (size_t i = 0; i < size; i++)
    {
        fputc(data[i], file);
    }
    fputc(STRING_QUOTE, file);
}

void write_code_point(FILE * file, const uint32_t ch)
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

void write_bracketed_code_point(FILE * file, const uint32_t ch)
{
    fputc(CODE_POINT_OPEN_BRACKET, file);
    write_code_point(file, ch);
    fputc(CODE_POINT_CLOSE_BRACKET, file);
}


void BoltValue_to_Char(struct BoltValue * value, uint32_t data)
{
    _format(value, BOLT_CHAR, 0, 1, NULL, 0);
    value->data.as_uint32[0] = data;
}

uint32_t BoltChar_get(const struct BoltValue * value)
{
    return value->data.as_uint32[0];
}

void BoltValue_to_String(struct BoltValue * value, const char * data, int32_t length)
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

void BoltValue_to_CharArray(struct BoltValue * value, const uint32_t * data, int32_t length)
{
    size_t data_size = length >= 0 ? sizeof(uint32_t) * length : 0;
    if (length <= sizeof(value->data) / sizeof(uint32_t))
    {
        _format(value, BOLT_CHAR_ARRAY, 0, length, NULL, 0);
        if (data != NULL)
        {
            memcpy(value->data.as_uint32, data, data_size);
        }
    }
    else if (BoltValue_type(value) == BOLT_CHAR_ARRAY)
    {
        value->data.extended.as_ptr = BoltMem_adjust(value->data.extended.as_ptr, value->data_size, data_size);
        value->data_size = data_size;
        value->size = length;
        if (data != NULL)
        {
            memcpy(value->data.extended.as_uint32, data, data_size);
        }
    }
    else
    {
        // If all else fails, allocate some new external data space
        _format(value, BOLT_CHAR_ARRAY, 0, length, data, data_size);
    }
}

void BoltValue_to_StringArray(struct BoltValue * value, int32_t length)
{
    _format(value, BOLT_STRING_ARRAY, 0, length, NULL, sizeof_n(struct array_t, length));
    for (int i = 0; i < length; i++)
    {
        value->data.extended.as_array[i].size = 0;
        value->data.extended.as_array[i].data.as_ptr = NULL;
    }
}

void BoltValue_to_Dictionary(struct BoltValue * value, int32_t length)
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

int BoltChar_write(const struct BoltValue * value, FILE * file)
{
    assert(BoltValue_type(value) == BOLT_CHAR);
    write_code_point(file, BoltChar_get(value));
    return 0;
}

int BoltCharArray_write(struct BoltValue * value, FILE * file)
{
    assert(BoltValue_type(value) == BOLT_CHAR_ARRAY);
    uint32_t * chars = BoltCharArray_get(value);
    fputc(CHAR_ARRAY_QUOTE, file);
    for (int32_t i = 0; i < value->size; i++)
    {
        uint32_t ch = chars[i];
        if (IS_PRINTABLE_ASCII(ch) && ch != CHAR_ARRAY_QUOTE &&
            ch != CODE_POINT_OPEN_BRACKET && ch != CODE_POINT_CLOSE_BRACKET)
        {
            fputc(ch, file);
        }
        else
        {
            write_bracketed_code_point(file, ch);
        }
    }
    fputc(CHAR_ARRAY_QUOTE, file);
    return 0;
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
            write_bracketed_code_point(file, (uint32_t)(ch0));
        }
        else if ((ch0 & 0b11100000) == 0b11000000)
        {
            if (i + 1 < value->size)
            {
                char ch1 = data[++i];
                uint32_t ch = ((ch0 & (uint32_t)(0b00011111)) << 6) | (ch1 & (uint32_t)(0b00111111));
                write_bracketed_code_point(file, ch);
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
                write_bracketed_code_point(file, ch);
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
                write_bracketed_code_point(file, ch);
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

int BoltStringArray_write(struct BoltValue * value, FILE * file)
{
    assert(BoltValue_type(value) == BOLT_STRING_ARRAY);
    fprintf(file, "$[");
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
            write_string(file, string.data.as_char, (size_t)(string.size));
        }
    }
    fprintf(file, "]");
    return 0;
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
            write_string(file, key, (size_t)(BoltDictionary_get_key_size(value, i)));
            fprintf(file, ": ");
            BoltValue_write(BoltDictionary_value(value, i), file, protocol_version);
            comma = 1;
        }
    }
    fprintf(file, "}");
    return 0;
}
