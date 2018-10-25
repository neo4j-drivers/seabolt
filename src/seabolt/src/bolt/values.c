/*
 * Copyright (c) 2002-2018 "Neo4j,"
 * Neo4j Sweden AB [http://neo4j.com]
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


#include "bolt-private.h"
#include "values-private.h"
#include "connection-private.h"
#include "mem.h"
#include "protocol.h"

#define STRING_QUOTE '"'
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
    if (type==BOLT_LIST || type==BOLT_STRUCTURE) {
        for (long i = 0; i<value->size; i++) {
            BoltValue_format_as_Null(&value->data.extended.as_value[i]);
        }
    }
    else if (type==BOLT_DICTIONARY) {
        for (long i = 0; i<2*value->size; i++) {
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
    if (size>value->size) {
        // grow physically
        size_t unit_size = sizeof(struct BoltValue);
        size_t new_data_size = multiplier*unit_size*size;
        size_t old_data_size = (size_t) value->data_size;
        value->data.extended.as_ptr = BoltMem_adjust(value->data.extended.as_ptr, (size_t) value->data_size,
                new_data_size);
        value->data_size = new_data_size;
        // grow logically
        memset(value->data.extended.as_char+old_data_size, 0, new_data_size-old_data_size);
        value->size = size;
    }
    else if (size<value->size) {
        // shrink logically
        for (long i = multiplier*size; i<multiplier*value->size; i++) {
            BoltValue_format_as_Null(&value->data.extended.as_value[i]);
        }
        value->size = size;
        // shrink physically
        size_t new_data_size = multiplier*sizeof_n(struct BoltValue, size);
        value->data.extended.as_ptr = BoltMem_adjust(value->data.extended.as_ptr, (size_t) value->data_size,
                new_data_size);
        value->data_size = new_data_size;
    }
    else {
        // same size - do nothing
    }
}

void _set_type(struct BoltValue* value, enum BoltType type, int16_t subtype, int32_t size)
{
    assert(((int) type)<0x80);
    value->type = (char) (type);
    value->subtype = subtype;
    value->size = size;
}

void
_format(struct BoltValue* value, enum BoltType type, int16_t subtype, int32_t size, const void* data, size_t data_size)
{
    _recycle(value);
    value->data.extended.as_ptr = BoltMem_adjust(value->data.extended.as_ptr, (size_t) value->data_size, data_size);
    value->data_size = data_size;
    if (data!=NULL && data_size>0) {
        memcpy(value->data.extended.as_char, data, data_size);
    }
    _set_type(value, type, subtype, size);
}

void _format_as_structure(struct BoltValue* value, enum BoltType type, int16_t code, int32_t size)
{
    _recycle(value);
    value->data.extended.as_ptr = BoltMem_adjust(value->data.extended.as_ptr, (size_t) value->data_size,
            sizeof_n(struct BoltValue, size));
    value->data_size = sizeof_n(struct BoltValue, size);
    memset(value->data.extended.as_char, 0, (size_t) value->data_size);
    _set_type(value, type, code, size);
}

void _write_escaped_code_point(FILE* file, const uint32_t ch)
{
    if (ch<0x10000) {
        fprintf(file, "\\u%c%c%c%c", hex3(&ch, 0), hex2(&ch, 0), hex1(&ch, 0), hex0(&ch, 0));
    }
    else if (ch<0x1000000) {
        fprintf(file, "\\U00%c%c%c%c%c%c", hex5(&ch, 0), hex4(&ch, 0), hex3(&ch, 0), hex2(&ch, 0), hex1(&ch, 0),
                hex0(&ch, 0));
    }
    else {
        fprintf(file, REPLACEMENT_CHARACTER);
    }
}

void _write_string(FILE* file, const char* data, size_t size)
{
    fputc(STRING_QUOTE, file);
    for (size_t i = 0; i<size; i++) {
        char ch0 = data[i];
        if (IS_PRINTABLE_ASCII(ch0) && ch0!=STRING_QUOTE) {
            fputc(ch0, file);
        }
        else if (ch0>=0) {
            _write_escaped_code_point(file, (uint32_t) (ch0));
        }
        else if ((ch0 & 0xE0)==0xC0) {
            if (i+1<size) {
                char ch1 = data[++i];
                uint32_t ch = ((ch0 & (uint32_t) (0x1F)) << 6) | (ch1 & (uint32_t) (0x3F));
                _write_escaped_code_point(file, ch);
            }
            else {
                fprintf(file, REPLACEMENT_CHARACTER);
            }
        }
        else if ((ch0 & 0xF0)==0xE0) {
            if (i+2<size) {
                char ch1 = data[++i];
                char ch2 = data[++i];
                uint32_t ch = ((ch0 & (uint32_t) (0x0F)) << 12) | ((ch1 & (uint32_t) (0x3F)) << 6) |
                        (ch2 & (uint32_t) (0x3F));
                _write_escaped_code_point(file, ch);
            }
            else {
                fprintf(file, REPLACEMENT_CHARACTER);
            }
        }
        else if ((ch0 & 0xF8)==0xF0) {
            if (i+3<size) {
                char ch1 = data[++i];
                char ch2 = data[++i];
                char ch3 = data[++i];
                uint32_t ch = ((ch0 & (uint32_t) (0x07)) << 18) | ((ch1 & (uint32_t) (0x3F)) << 12) |
                        ((ch2 & (uint32_t) (0x3F)) << 6) | (ch3 & (uint32_t) (0x3F));
                _write_escaped_code_point(file, ch);
            }
            else {
                fprintf(file, REPLACEMENT_CHARACTER);
            }
        }
        else {
            fprintf(file, REPLACEMENT_CHARACTER);
        }
    }
    fputc(STRING_QUOTE, file);
}

struct BoltValue* BoltValue_create()
{
    size_t size = sizeof(struct BoltValue);
    struct BoltValue* value = (struct BoltValue*) BoltMem_allocate(size);
    _set_type(value, BOLT_NULL, 0, 0);
    value->data_size = 0;
    value->data.as_int64[0] = 0;
    value->data.as_int64[1] = 0;
    value->data.extended.as_ptr = NULL;
    return value;
}

void BoltValue_destroy(struct BoltValue* value)
{
    if (value==NULL) {
        return;
    }

    BoltValue_format_as_Null(value);
    BoltMem_deallocate(value, sizeof(struct BoltValue));
}

struct BoltValue* BoltValue_duplicate(const struct BoltValue* value)
{
    if (value==NULL) {
        return NULL;
    }

    struct BoltValue* duplicate = BoltValue_create();
    BoltValue_copy(duplicate, value);
    return duplicate;
}

void BoltValue_copy(struct BoltValue* dest, const struct BoltValue* src)
{
    assert(src!=NULL);
    assert(dest!=NULL);

    _recycle(dest);

    switch (src->type) {
    case BOLT_NULL:
        BoltValue_format_as_Null(dest);
        break;
    case BOLT_BOOLEAN:
        BoltValue_format_as_Boolean(dest, BoltBoolean_get(src));
        break;
    case BOLT_INTEGER:
        BoltValue_format_as_Integer(dest, BoltInteger_get(src));
        break;
    case BOLT_FLOAT:
        BoltValue_format_as_Float(dest, BoltFloat_get(src));
        break;
    case BOLT_STRING:
        BoltValue_format_as_String(dest, BoltString_get(src), src->size);
        break;
    case BOLT_DICTIONARY:
        BoltValue_format_as_Dictionary(dest, src->size);
        for (int i = 0; i<src->size; i++) {
            struct BoltValue* dest_key = BoltDictionary_key(dest, i);
            struct BoltValue* dest_value = BoltDictionary_value(dest, i);

            BoltValue_copy(dest_key, BoltDictionary_key(src, i));
            BoltValue_copy(dest_value, BoltDictionary_value(src, i));
        }
        break;
    case BOLT_LIST:
        BoltValue_format_as_List(dest, src->size);
        for (int i = 0; i<src->size; i++) {
            struct BoltValue* dest_element = BoltList_value(dest, i);

            BoltValue_copy(dest_element, BoltList_value(src, i));
        }
        break;
    case BOLT_BYTES:
        BoltValue_format_as_Bytes(dest, BoltBytes_get_all(src), src->size);
        break;
    case BOLT_STRUCTURE:
        BoltValue_format_as_Structure(dest, src->subtype, src->size);
        for (int i = 0; i<src->size; i++) {
            struct BoltValue* dest_element = BoltStructure_value(dest, i);

            BoltValue_copy(dest_element, BoltStructure_value(src, i));
        }
        break;
    default:
        assert(0);
    }
}

int32_t BoltValue_size(const struct BoltValue* value)
{
    return value->size;
}

enum BoltType BoltValue_type(const struct BoltValue* value)
{
    return (enum BoltType) (value->type);
}

int32_t BoltValue_string(const struct BoltValue* value, char* dest, int32_t length, struct BoltConnection* connection)
{
    name_resolver_func name_resolver = NULL;
    struct StringBuilder* builder = StringBuilder_create();
    if (connection!=NULL && connection->protocol!=NULL) {
        name_resolver = connection->protocol->structure_name;
    }

    int32_t result = BoltValue_write(builder, value, name_resolver);
    if (result==BOLT_SUCCESS) {
        int32_t builder_length = StringBuilder_get_length(builder);
        int32_t copy_length = builder_length>length ? length : builder_length;
        strncpy(dest, StringBuilder_get_string(builder), copy_length);
        result = builder_length;
    }

    StringBuilder_destroy(builder);

    return result;
}

void BoltValue_format_as_Null(struct BoltValue* value)
{
    if (BoltValue_type(value)!=BOLT_NULL) {
        _format(value, BOLT_NULL, 0, 0, NULL, 0);
    }
}

void BoltValue_format_as_Boolean(struct BoltValue* value, char data)
{
    _format(value, BOLT_BOOLEAN, 0, 1, NULL, 0);
    value->data.as_char[0] = data;
}

char BoltBoolean_get(const struct BoltValue* value)
{
    return to_bit(value->data.as_char[0]);
}

void BoltValue_format_as_Integer(struct BoltValue* value, int64_t data)
{
    _format(value, BOLT_INTEGER, 0, 1, NULL, 0);
    value->data.as_int64[0] = data;
}

int64_t BoltInteger_get(const struct BoltValue* value)
{
    return value->data.as_int64[0];
}

void BoltValue_format_as_Float(struct BoltValue* value, double data)
{
    _format(value, BOLT_FLOAT, 0, 1, NULL, 0);
    value->data.as_double[0] = data;
}

double BoltFloat_get(const struct BoltValue* value)
{
    return value->data.as_double[0];
}

void BoltValue_format_as_String(struct BoltValue* value, const char* data, int32_t length)
{
    size_t data_size = length>=0 ? sizeof(char)*length : 0;
    if (length<=(int32_t) (sizeof(value->data)/sizeof(char))) {
        // If the string is short, it can fit entirely within the
        // BoltValue instance
        _format(value, BOLT_STRING, 0, length, NULL, 0);
        if (data!=NULL) {
            memcpy(value->data.as_char, data, (size_t) (length));
        }
    }
    else if (BoltValue_type(value)==BOLT_STRING) {
        // This is already a UTF-8 string so we can just tweak the value
        value->data.extended.as_ptr = BoltMem_adjust(value->data.extended.as_ptr, (size_t) value->data_size, data_size);
        value->data_size = data_size;
        value->size = length;
        if (data!=NULL) {
            memcpy(value->data.extended.as_char, data, data_size);
        }
    }
    else {
        // If all else fails, allocate some new external data space
        _format(value, BOLT_STRING, 0, length, data, data_size);
    }
}

char* BoltString_get(const struct BoltValue* value)
{
    return value->size<=(int32_t) (sizeof(value->data)/sizeof(char)) ?
           (char*) value->data.as_char : value->data.extended.as_char;
}

int BoltString_equals(struct BoltValue* value, const char* data, const size_t data_size)
{
    if (BoltValue_type(value)==BOLT_STRING) {
        const int32_t length = (int32_t) data_size;
        if (value->size!=length) {
            return 0;
        }

        const char* value_data = BoltString_get(value);
        if (strncmp(value_data, data, length)==0) {
            return 1;
        }
    }

    return 0;
}

void BoltValue_format_as_Dictionary(struct BoltValue* value, int32_t length)
{
    if (value->type==BOLT_DICTIONARY) {
        _resize(value, length, 2);
    }
    else {
        size_t unit_size = sizeof(struct BoltValue);
        size_t data_size = 2*unit_size*length;
        _recycle(value);
        value->data.extended.as_ptr = BoltMem_adjust(value->data.extended.as_ptr, (size_t) value->data_size, data_size);
        value->data_size = data_size;
        memset(value->data.extended.as_char, 0, data_size);
        _set_type(value, BOLT_DICTIONARY, 0, length);
    }
}

struct BoltValue* BoltDictionary_key(const struct BoltValue* value, int32_t index)
{
    assert(BoltValue_type(value)==BOLT_DICTIONARY);
    return &value->data.extended.as_value[2*index];
}

const char* BoltDictionary_get_key(const struct BoltValue* value, int32_t index)
{
    assert(BoltValue_type(value)==BOLT_DICTIONARY);
    struct BoltValue* key_value = &value->data.extended.as_value[2*index];
    assert(BoltValue_type(key_value)==BOLT_STRING);
    return BoltString_get(key_value);
}

int32_t BoltDictionary_get_key_size(const struct BoltValue* value, int32_t index)
{
    assert(BoltValue_type(value)==BOLT_DICTIONARY);
    struct BoltValue* key_value = &value->data.extended.as_value[2*index];
    assert(BoltValue_type(key_value)==BOLT_STRING);
    return key_value->size;
}

int32_t
BoltDictionary_get_key_index(const struct BoltValue* value, const char* key, size_t key_size, int32_t start_index)
{
    assert(BoltValue_type(value)==BOLT_DICTIONARY);
    if (start_index>=value->size) return -1;
    for (int32_t i = start_index; i<value->size; i++) {
        struct BoltValue* key_value = &value->data.extended.as_value[2*i];
        if (key_value->size==(int32_t) key_size) {
            if (strncmp(BoltString_get(key_value), key, key_size)==0) {
                return i;
            }
        }
    }
    return -1;
}

int BoltDictionary_set_key(struct BoltValue* value, int32_t index, const char* key, size_t key_size)
{
    if (key_size<=INT32_MAX) {
        assert(BoltValue_type(value)==BOLT_DICTIONARY);
        BoltValue_format_as_String(&value->data.extended.as_value[2*index], key, (int32_t) key_size);
        return 0;
    }
    else {
        return -1;
    }
}

struct BoltValue* BoltDictionary_value(const struct BoltValue* value, int32_t index)
{
    assert(BoltValue_type(value)==BOLT_DICTIONARY);
    return &value->data.extended.as_value[2*index+1];
}

struct BoltValue* BoltDictionary_value_by_key(const struct BoltValue* value, const char* key, size_t key_size)
{
    const int32_t index = BoltDictionary_get_key_index(value, key, key_size, 0);
    if (index<0) {
        return NULL;
    }

    return BoltDictionary_value(value, index);
}

void BoltValue_format_as_List(struct BoltValue* value, int32_t length)
{
    if (BoltValue_type(value)==BOLT_LIST) {
        BoltList_resize(value, length);
    }
    else {
        size_t data_size = sizeof(struct BoltValue)*length;
        _recycle(value);
        value->data.extended.as_ptr = BoltMem_adjust(value->data.extended.as_ptr, (size_t) value->data_size, data_size);
        value->data_size = data_size;
        memset(value->data.extended.as_char, 0, data_size);
        _set_type(value, BOLT_LIST, 0, length);
    }
}

void BoltList_resize(struct BoltValue* value, int32_t size)
{
    assert(BoltValue_type(value)==BOLT_LIST);
    _resize(value, size, 1);
}

struct BoltValue* BoltList_value(const struct BoltValue* value, int32_t index)
{
    assert(BoltValue_type(value)==BOLT_LIST);
    return &value->data.extended.as_value[index];
}

void BoltValue_format_as_Bytes(struct BoltValue* value, char* data, int32_t length)
{
    if (length<=(int32_t) (sizeof(value->data)/sizeof(char))) {
        _format(value, BOLT_BYTES, 0, length, NULL, 0);
        if (data!=NULL) {
            memcpy(value->data.as_char, data, (size_t) (length));
        }
    }
    else {
        _format(value, BOLT_BYTES, 0, length, data, sizeof_n(char, length));
    }
}

char BoltBytes_get(const struct BoltValue* value, int32_t index)
{
    const char* data = value->size<=(int32_t) (sizeof(value->data)/sizeof(char)) ?
                       value->data.as_char : value->data.extended.as_char;
    return data[index];
}

char* BoltBytes_get_all(const struct BoltValue* value)
{
    return value->size<=(int32_t) (sizeof(value->data)/sizeof(char)) ?
           (char*) value->data.as_char : value->data.extended.as_char;
}

void BoltValue_format_as_Structure(struct BoltValue* value, int16_t code, int32_t length)
{
    _format_as_structure(value, BOLT_STRUCTURE, code, length);
}

int16_t BoltStructure_code(const struct BoltValue* value)
{
    assert(BoltValue_type(value)==BOLT_STRUCTURE);
    return value->subtype;
}

struct BoltValue* BoltStructure_value(const struct BoltValue* value, int32_t index)
{
    assert(BoltValue_type(value)==BOLT_STRUCTURE);
    return &value->data.extended.as_value[index];
}

int
BoltValue_write(struct StringBuilder* builder, const struct BoltValue* value, name_resolver_func struct_name_resolver)
{
    switch (BoltValue_type(value)) {
    case BOLT_NULL: {
        StringBuilder_append(builder, "null");
        return 0;
    }
    case BOLT_BOOLEAN: {
        StringBuilder_append_f(builder, "%s", BoltBoolean_get(value) ? "true" : "false");
        return 0;
    }
    case BOLT_INTEGER: {
        StringBuilder_append_f(builder, "%" PRIi64, BoltInteger_get(value));
        return 0;
    }
    case BOLT_FLOAT: {
        StringBuilder_append_f(builder, "%E", BoltFloat_get(value));
        return 0;
    }
    case BOLT_STRING: {
        char* data = BoltString_get(value);
        if (value->size>=0) {
            StringBuilder_append_n(builder, data, value->size);
            return 0;
        }
        else {
            return -1;
        }
    }
    case BOLT_DICTIONARY: {
        StringBuilder_append(builder, "{");
        int comma = 0;
        for (int i = 0; i<value->size; i++) {
            struct BoltValue* key = BoltDictionary_key(value, i);
            if (key!=NULL) {
                if (comma) StringBuilder_append(builder, ", ");
                StringBuilder_append_n(builder, BoltString_get(key), key->size);
                StringBuilder_append(builder, ": ");
                BoltValue_write(builder, BoltDictionary_value(value, i), struct_name_resolver);
                comma = 1;
            }
        }
        StringBuilder_append(builder, "}");
        return 0;
    }
    case BOLT_LIST: {
        StringBuilder_append(builder, "[");
        for (int i = 0; i<value->size; i++) {
            if (i>0) StringBuilder_append(builder, ", ");
            BoltValue_write(builder, BoltList_value(value, i), struct_name_resolver);
        }
        StringBuilder_append(builder, "]");
        return 0;
    }
    case BOLT_BYTES: {
        StringBuilder_append(builder, "#");
        for (int i = 0; i<value->size; i++) {
            char b = BoltBytes_get(value, i);
            StringBuilder_append_f(builder, "%c%c", hex1(&b, 0), hex0(&b, 0));
        }
        return 0;
    }
    case BOLT_STRUCTURE: {
        int16_t code = BoltStructure_code(value);

        if (struct_name_resolver!=NULL) {
            StringBuilder_append_f(builder, "$%s", struct_name_resolver(code));
        }
        else {
            StringBuilder_append_f(builder, "$#%c%c%c%c", hex3(&code, 0), hex2(&code, 0), hex1(&code, 0),
                    hex0(&code, 0));
        }

        StringBuilder_append(builder, "(");
        for (int i = 0; i<value->size; i++) {
            if (i>0) StringBuilder_append(builder, " ");
            BoltValue_write(builder, BoltStructure_value(value, i), struct_name_resolver);
        }
        StringBuilder_append(builder, ")");
        return 0;
    }
    default:
        StringBuilder_append(builder, "?");
        return -1;
    }
}
