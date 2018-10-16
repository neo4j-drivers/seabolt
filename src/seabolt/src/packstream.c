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

#include "config-impl.h"
#include "buffering.h"
#include "connections.h"
#include "logging.h"
#include "packstream.h"

#define TRY(code) { int status_try = (code); if (status_try != BOLT_SUCCESS) { return status_try; } }

int load_null(struct BoltBuffer* buffer)
{
    BoltBuffer_load_u8(buffer, 0xC0);
    return BOLT_SUCCESS;
}

int load_boolean(struct BoltBuffer* buffer, int value)
{
    BoltBuffer_load_u8(buffer, (value==0) ? (uint8_t) (0xC2) : (uint8_t) (0xC3));
    return BOLT_SUCCESS;
}

int load_integer(struct BoltBuffer* buffer, int64_t value)
{
    if (value>=-0x10 && value<0x80) {
        BoltBuffer_load_i8(buffer, (int8_t) (value));
    }
    else if (value>=INT8_MIN && value<=INT8_MAX) {
        BoltBuffer_load_u8(buffer, 0xC8);
        BoltBuffer_load_i8(buffer, (int8_t) (value));
    }
    else if (value>=INT16_MIN && value<=INT16_MAX) {
        BoltBuffer_load_u8(buffer, 0xC9);
        BoltBuffer_load_i16be(buffer, (int16_t) (value));
    }
    else if (value>=INT32_MIN && value<=INT32_MAX) {
        BoltBuffer_load_u8(buffer, 0xCA);
        BoltBuffer_load_i32be(buffer, (int32_t) (value));
    }
    else {
        BoltBuffer_load_u8(buffer, 0xCB);
        BoltBuffer_load_i64be(buffer, value);
    }
    return BOLT_SUCCESS;
}

int load_float(struct BoltBuffer* buffer, double value)
{
    BoltBuffer_load_u8(buffer, 0xC1);
    BoltBuffer_load_f64be(buffer, value);
    return BOLT_SUCCESS;
}

int load_bytes(struct BoltBuffer* buffer, const char* string, int32_t size)
{
    if (size<0) {
        return BOLT_PROTOCOL_VIOLATION;
    }
    if (size<0x100) {
        BoltBuffer_load_u8(buffer, 0xCC);
        BoltBuffer_load_u8(buffer, (uint8_t) (size));
        BoltBuffer_load(buffer, string, size);
    }
    else if (size<0x10000) {
        BoltBuffer_load_u8(buffer, 0xCD);
        BoltBuffer_load_u16be(buffer, (uint16_t) (size));
        BoltBuffer_load(buffer, string, size);
    }
    else {
        BoltBuffer_load_u8(buffer, 0xCE);
        BoltBuffer_load_i32be(buffer, size);
        BoltBuffer_load(buffer, string, size);
    }
    return BOLT_SUCCESS;
}

int load_string_header(struct BoltBuffer* buffer, int32_t size)
{
    if (size<0) {
        return BOLT_PROTOCOL_VIOLATION;
    }
    if (size<0x10) {
        BoltBuffer_load_u8(buffer, (uint8_t) (0x80+size));
    }
    else if (size<0x100) {
        BoltBuffer_load_u8(buffer, 0xD0);
        BoltBuffer_load_u8(buffer, (uint8_t) (size));
    }
    else if (size<0x10000) {
        BoltBuffer_load_u8(buffer, 0xD1);
        BoltBuffer_load_u16be(buffer, (uint16_t) (size));
    }
    else {
        BoltBuffer_load_u8(buffer, 0xD2);
        BoltBuffer_load_i32be(buffer, size);
    }
    return BOLT_SUCCESS;
}

int load_string(struct BoltBuffer* buffer, const char* string, int32_t size)
{
    int status = load_string_header(buffer, size);
    if (status!=BOLT_SUCCESS) return status;
    BoltBuffer_load(buffer, string, size);
    return BOLT_SUCCESS;
}

int load_list_header(struct BoltBuffer* buffer, int32_t size)
{
    if (size<0) {
        return BOLT_PROTOCOL_VIOLATION;
    }
    if (size<0x10) {
        BoltBuffer_load_u8(buffer, (uint8_t) (0x90+size));
    }
    else if (size<0x100) {
        BoltBuffer_load_u8(buffer, 0xD4);
        BoltBuffer_load_u8(buffer, (uint8_t) (size));
    }
    else if (size<0x10000) {
        BoltBuffer_load_u8(buffer, 0xD5);
        BoltBuffer_load_u16be(buffer, (uint16_t) (size));
    }
    else {
        BoltBuffer_load_u8(buffer, 0xD6);
        BoltBuffer_load_i32be(buffer, size);
    }
    return BOLT_SUCCESS;
}

int load_map_header(struct BoltBuffer* buffer, int32_t size)
{
    if (size<0) {
        return BOLT_PROTOCOL_VIOLATION;
    }
    if (size<0x10) {
        BoltBuffer_load_u8(buffer, (uint8_t) (0xA0+size));
    }
    else if (size<0x100) {
        BoltBuffer_load_u8(buffer, 0xD8);
        BoltBuffer_load_u8(buffer, (uint8_t) (size));
    }
    else if (size<0x10000) {
        BoltBuffer_load_u8(buffer, 0xD9);
        BoltBuffer_load_u16be(buffer, (uint16_t) (size));
    }
    else {
        BoltBuffer_load_u8(buffer, 0xDA);
        BoltBuffer_load_i32be(buffer, size);
    }
    return BOLT_SUCCESS;
}

int load_structure_header(struct BoltBuffer* buffer, int16_t code, int8_t size)
{
    if (code<0 || size<0 || size>=0x10) {
        return BOLT_PROTOCOL_VIOLATION;
    }
    BoltBuffer_load_u8(buffer, (uint8_t) (0xB0+size));
    BoltBuffer_load_i8(buffer, (int8_t) (code));
    return BOLT_SUCCESS;
}

int load(check_struct_signature_func check_struct_type, struct BoltBuffer* buffer, struct BoltValue* value,
        const struct BoltLog* log)
{
    switch (BoltValue_type(value)) {
    case BOLT_NULL:
        return load_null(buffer);
    case BOLT_LIST: {
        TRY(load_list_header(buffer, value->size));
        for (int32_t i = 0; i<value->size; i++) {
            TRY(load(check_struct_type, buffer, BoltList_value(value, i), log));
        }
        return 0;
    }
    case BOLT_BOOLEAN:
        return load_boolean(buffer, BoltBoolean_get(value));
    case BOLT_BYTES:
        return load_bytes(buffer, BoltBytes_get_all(value), value->size);
    case BOLT_STRING:
        return load_string(buffer, BoltString_get(value), value->size);
    case BOLT_DICTIONARY: {
        TRY(load_map_header(buffer, value->size));
        for (int32_t i = 0; i<value->size; i++) {
            const char* key = BoltDictionary_get_key(value, i);
            if (key!=NULL) {
                TRY(load_string(buffer, key, BoltDictionary_get_key_size(value, i)));
                TRY(load(check_struct_type, buffer, BoltDictionary_value(value, i), log));
            }
        }
        return BOLT_SUCCESS;
    }
    case BOLT_INTEGER:
        return load_integer(buffer, BoltInteger_get(value));
    case BOLT_FLOAT:
        return load_float(buffer, BoltFloat_get(value));
    case BOLT_STRUCTURE: {
        if (check_struct_type(BoltStructure_code(value))) {
            TRY(load_structure_header(buffer, BoltStructure_code(value), (int8_t) value->size));
            for (int32_t i = 0; i<value->size; i++) {
                TRY(load(check_struct_type, buffer, BoltStructure_value(value, i), log));
            }
            return BOLT_SUCCESS;
        }
        return BOLT_PROTOCOL_UNSUPPORTED_TYPE;
    }
    default:
        return BOLT_PROTOCOL_NOT_IMPLEMENTED_TYPE;
    }
}

enum PackStreamType marker_type(uint8_t marker)
{
    if (marker<0x80 || (marker>=0xC8 && marker<=0xCB) || marker>=0xF0) {
        return PACKSTREAM_INTEGER;
    }
    if ((marker>=0x80 && marker<=0x8F) || (marker>=0xD0 && marker<=0xD2)) {
        return PACKSTREAM_STRING;
    }
    if ((marker>=0x90 && marker<=0x9F) || (marker>=0xD4 && marker<=0xD6)) {
        return PACKSTREAM_LIST;
    }
    if ((marker>=0xA0 && marker<=0xAF) || (marker>=0xD8 && marker<=0xDA)) {
        return PACKSTREAM_MAP;
    }
    if ((marker>=0xB0 && marker<=0xBF) || (marker>=0xDC && marker<=0xDD)) {
        return PACKSTREAM_STRUCTURE;
    }
    switch (marker) {
    case 0xC0:
        return PACKSTREAM_NULL;
    case 0xC1:
        return PACKSTREAM_FLOAT;
    case 0xC2:
    case 0xC3:
        return PACKSTREAM_BOOLEAN;
    case 0xCC:
    case 0xCD:
    case 0xCE:
        return PACKSTREAM_BYTES;
    default:
        return PACKSTREAM_RESERVED;
    }
}

int unload_null(struct BoltBuffer* recv_buffer, struct BoltValue* value)
{
    uint8_t marker;
    BoltBuffer_unload_u8(recv_buffer, &marker);
    if (marker==0xC0) {
        BoltValue_format_as_Null(value);
    }
    else {
        return BOLT_PROTOCOL_UNEXPECTED_MARKER;
    }
    return BOLT_SUCCESS;
}

int unload_boolean(struct BoltBuffer* recv_buffer, struct BoltValue* value)
{
    uint8_t marker;
    BoltBuffer_unload_u8(recv_buffer, &marker);
    if (marker==0xC3) {
        BoltValue_format_as_Boolean(value, 1);
    }
    else if (marker==0xC2) {
        BoltValue_format_as_Boolean(value, 0);
    }
    else {
        return BOLT_PROTOCOL_UNEXPECTED_MARKER;
    }
    return BOLT_SUCCESS;
}

int unload_integer(struct BoltBuffer* recv_buffer, struct BoltValue* value)
{
    uint8_t marker;
    BoltBuffer_unload_u8(recv_buffer, &marker);
    if (marker<0x80) {
        BoltValue_format_as_Integer(value, marker);
    }
    else if (marker>=0xF0) {
        BoltValue_format_as_Integer(value, marker-0x100);
    }
    else if (marker==0xC8) {
        int8_t x;
        BoltBuffer_unload_i8(recv_buffer, &x);
        BoltValue_format_as_Integer(value, x);
    }
    else if (marker==0xC9) {
        int16_t x;
        BoltBuffer_unload_i16be(recv_buffer, &x);
        BoltValue_format_as_Integer(value, x);
    }
    else if (marker==0xCA) {
        int32_t x;
        BoltBuffer_unload_i32be(recv_buffer, &x);
        BoltValue_format_as_Integer(value, x);
    }
    else if (marker==0xCB) {
        int64_t x;
        BoltBuffer_unload_i64be(recv_buffer, &x);
        BoltValue_format_as_Integer(value, x);
    }
    else {
        return BOLT_PROTOCOL_UNEXPECTED_MARKER;  // BOLT_ERROR_WRONG_TYPE
    }
    return BOLT_SUCCESS;
}

int unload_float(struct BoltBuffer* recv_buffer, struct BoltValue* value)
{
    uint8_t marker;
    BoltBuffer_unload_u8(recv_buffer, &marker);
    if (marker==0xC1) {
        double x;
        BoltBuffer_unload_f64be(recv_buffer, &x);
        BoltValue_format_as_Float(value, x);
    }
    else {
        return BOLT_PROTOCOL_UNEXPECTED_MARKER;  // BOLT_ERROR_WRONG_TYPE
    }
    return BOLT_SUCCESS;
}

int unload_string(struct BoltBuffer* recv_buffer, struct BoltValue* value, const struct BoltLog* log)
{
    uint8_t marker;
    BoltBuffer_unload_u8(recv_buffer, &marker);
    if (marker>=0x80 && marker<=0x8F) {
        int32_t size;
        size = marker & 0x0F;
        BoltValue_format_as_String(value, NULL, size);
        BoltBuffer_unload(recv_buffer, BoltString_get(value), size);
        return BOLT_SUCCESS;
    }
    if (marker==0xD0) {
        uint8_t size;
        BoltBuffer_unload_u8(recv_buffer, &size);
        BoltValue_format_as_String(value, NULL, size);
        BoltBuffer_unload(recv_buffer, BoltString_get(value), size);
        return BOLT_SUCCESS;
    }
    if (marker==0xD1) {
        uint16_t size;
        BoltBuffer_unload_u16be(recv_buffer, &size);
        BoltValue_format_as_String(value, NULL, size);
        BoltBuffer_unload(recv_buffer, BoltString_get(value), size);
        return BOLT_SUCCESS;
    }
    if (marker==0xD2) {
        int32_t size;
        BoltBuffer_unload_i32be(recv_buffer, &size);
        BoltValue_format_as_String(value, NULL, size);
        BoltBuffer_unload(recv_buffer, BoltString_get(value), size);
        return BOLT_SUCCESS;
    }
    BoltLog_error(log, "Unknown marker: %d", marker);
    return BOLT_PROTOCOL_UNEXPECTED_MARKER;
}

int unload_bytes(struct BoltBuffer* recv_buffer, struct BoltValue* value, const struct BoltLog* log)
{
    uint8_t marker;
    BoltBuffer_unload_u8(recv_buffer, &marker);
    if (marker==0xCC) {
        uint8_t size;
        BoltBuffer_unload_u8(recv_buffer, &size);
        BoltValue_format_as_Bytes(value, NULL, size);
        BoltBuffer_unload(recv_buffer, BoltBytes_get_all(value), size);
        return BOLT_SUCCESS;
    }
    if (marker==0xCD) {
        uint16_t size;
        BoltBuffer_unload_u16be(recv_buffer, &size);
        BoltValue_format_as_Bytes(value, NULL, size);
        BoltBuffer_unload(recv_buffer, BoltBytes_get_all(value), size);
        return BOLT_SUCCESS;
    }
    if (marker==0xCE) {
        int32_t size;
        BoltBuffer_unload_i32be(recv_buffer, &size);
        BoltValue_format_as_Bytes(value, NULL, size);
        BoltBuffer_unload(recv_buffer, BoltBytes_get_all(value), size);
        return BOLT_SUCCESS;
    }
    BoltLog_error(log, "Unknown marker: %d", marker);
    return BOLT_PROTOCOL_UNEXPECTED_MARKER;
}

int unload_list(check_struct_signature_func check_struct_type, struct BoltBuffer* recv_buffer, struct BoltValue* value,
        const struct BoltLog* log)
{
    uint8_t marker;
    int32_t size;
    BoltBuffer_unload_u8(recv_buffer, &marker);
    if (marker>=0x90 && marker<=0x9F) {
        size = marker & 0x0F;
    }
    else if (marker==0xD4) {
        uint8_t size_;
        BoltBuffer_unload_u8(recv_buffer, &size_);
        size = size_;
    }
    else if (marker==0xD5) {
        uint16_t size_;
        BoltBuffer_unload_u16be(recv_buffer, &size_);
        size = size_;
    }
    else if (marker==0xD6) {
        int32_t size_;
        BoltBuffer_unload_i32be(recv_buffer, &size_);
        size = size_;
    }
    else {
        return BOLT_PROTOCOL_UNEXPECTED_MARKER;
    }
    if (size<0) {
        return BOLT_PROTOCOL_VIOLATION;
    }
    BoltValue_format_as_List(value, size);
    for (int i = 0; i<size; i++) {
        TRY(unload(check_struct_type, recv_buffer, BoltList_value(value, i), log));
    }
    return BOLT_SUCCESS;
}

int unload_map(check_struct_signature_func check_struct_type, struct BoltBuffer* recv_buffer, struct BoltValue* value,
        const struct BoltLog* log)
{
    uint8_t marker;
    int32_t size;
    BoltBuffer_unload_u8(recv_buffer, &marker);
    if (marker>=0xA0 && marker<=0xAF) {
        size = marker & 0x0F;
    }
    else if (marker==0xD8) {
        uint8_t size_;
        BoltBuffer_unload_u8(recv_buffer, &size_);
        size = size_;
    }
    else if (marker==0xD9) {
        uint16_t size_;
        BoltBuffer_unload_u16be(recv_buffer, &size_);
        size = size_;
    }
    else if (marker==0xDA) {
        int32_t size_;
        BoltBuffer_unload_i32be(recv_buffer, &size_);
        size = size_;
    }
    else {
        return BOLT_PROTOCOL_UNEXPECTED_MARKER;
    }
    if (size<0) {
        return BOLT_PROTOCOL_VIOLATION;
    }
    BoltValue_format_as_Dictionary(value, size);
    for (int i = 0; i<size; i++) {
        TRY(unload(check_struct_type, recv_buffer, BoltDictionary_key(value, i), log));
        TRY(unload(check_struct_type, recv_buffer, BoltDictionary_value(value, i), log));
    }
    return BOLT_SUCCESS;
}

int
unload_structure(check_struct_signature_func check_struct_type, struct BoltBuffer* recv_buffer, struct BoltValue* value,
        const struct BoltLog* log)
{
    uint8_t marker;
    int8_t code;
    int32_t size;
    BoltBuffer_unload_u8(recv_buffer, &marker);
    if (marker>=0xB0 && marker<=0xBF) {
        size = marker & 0x0F;
        BoltBuffer_unload_i8(recv_buffer, &code);
        if (check_struct_type(code)) {
            BoltValue_format_as_Structure(value, code, size);
            for (int i = 0; i<size; i++) {
                unload(check_struct_type, recv_buffer, BoltStructure_value(value, i), log);
            }
            return BOLT_SUCCESS;
        }
    }
    return BOLT_PROTOCOL_UNEXPECTED_MARKER;
}

int unload(check_struct_signature_func check_struct_type, struct BoltBuffer* buffer, struct BoltValue* value,
        const struct BoltLog* log)
{
    uint8_t marker;
    BoltBuffer_peek_u8(buffer, &marker);
    switch (marker_type(marker)) {
    case PACKSTREAM_NULL:
        return unload_null(buffer, value);
    case PACKSTREAM_BOOLEAN:
        return unload_boolean(buffer, value);
    case PACKSTREAM_INTEGER:
        return unload_integer(buffer, value);
    case PACKSTREAM_FLOAT:
        return unload_float(buffer, value);
    case PACKSTREAM_STRING:
        return unload_string(buffer, value, log);
    case PACKSTREAM_BYTES:
        return unload_bytes(buffer, value, log);
    case PACKSTREAM_LIST:
        return unload_list(check_struct_type, buffer, value, log);
    case PACKSTREAM_MAP:
        return unload_map(check_struct_type, buffer, value, log);
    case PACKSTREAM_STRUCTURE:
        return unload_structure(check_struct_type, buffer, value, log);
    default:
        BoltLog_error(log, "Unknown marker: %d", marker);
        return BOLT_PROTOCOL_UNEXPECTED_MARKER;
    }
}
