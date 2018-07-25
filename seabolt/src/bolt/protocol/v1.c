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
#include <memory.h>
#include <string.h>
#include <stdlib.h>
#include <bolt/connections.h>

#include "bolt/buffering.h"
#include "bolt/logging.h"
#include "bolt/mem.h"
#include "v1.h"

#define INIT        0x01
#define ACK_FAILURE 0x0E
#define RESET       0x0F
#define RUN         0x10
#define DISCARD_ALL 0x2F
#define PULL_ALL    0x3F

#define INITIAL_TX_BUFFER_SIZE 8192
#define INITIAL_RX_BUFFER_SIZE 8192

#define MAX_BOOKMARK_SIZE 40
#define MAX_SERVER_SIZE 200

#define MAX_LOGGED_RECORDS 3

#define char_to_uint16be(array) ((uint8_t)(header[0]) << 8) | (uint8_t)(header[1]);

#define TRY(code) { int status = (code); if (status == -1) { return status; } }

struct BoltMessage* BoltMessage_create(int8_t code, int32_t n_fields)
{
    const size_t size = sizeof(struct BoltMessage);
    struct BoltMessage* message = BoltMem_allocate(size);
    message->code = code;
    message->fields = BoltValue_create();
    BoltValue_format_as_List(message->fields, n_fields);
    return message;
}

void BoltMessage_destroy(struct BoltMessage* message)
{
    BoltValue_destroy(message->fields);
    BoltMem_deallocate(message, sizeof(struct BoltMessage));
}

int BoltProtocolV1_compile_INIT(struct BoltMessage* message, const char* user_agent, const struct BoltValue* auth_token,
        int mask_secure_fields)
{
    struct BoltValue* user_agent_field = BoltList_value(message->fields, 0);
    struct BoltValue* auth_token_field = BoltList_value(message->fields, 1);

    BoltValue_format_as_String(user_agent_field, user_agent, (int32_t) (strlen(user_agent)));
    BoltValue_copy(auth_token_field, auth_token);

    if (mask_secure_fields) {
        struct BoltValue* secure_value = BoltDictionary_value_by_key(auth_token_field, "credentials", 11);
        if (secure_value!=NULL) {
            BoltValue_format_as_String(secure_value, "********", 8);
        }
    }

    return 0;
}

void compile_RUN(struct _run_request* run, int32_t n_parameters)
{
    run->request = BoltMessage_create(RUN, 2);
    run->statement = BoltList_value(run->request->fields, 0);
    run->parameters = BoltList_value(run->request->fields, 1);
    BoltValue_format_as_Dictionary(run->parameters, n_parameters);
}

struct BoltProtocolV1State* BoltProtocolV1_create_state()
{
    struct BoltProtocolV1State* state = BoltMem_allocate(sizeof(struct BoltProtocolV1State));

    state->tx_buffer = BoltBuffer_create(INITIAL_TX_BUFFER_SIZE);
    state->rx_buffer = BoltBuffer_create(INITIAL_RX_BUFFER_SIZE);

    state->server = BoltMem_allocate(MAX_SERVER_SIZE);
    memset(state->server, 0, MAX_SERVER_SIZE);
    state->result_field_names = BoltValue_create();
    state->result_metadata = BoltValue_create();
    BoltValue_format_as_Dictionary(state->result_metadata, 0);
    state->failure_data = NULL;
    state->last_bookmark = BoltMem_allocate(MAX_BOOKMARK_SIZE);
    memset(state->last_bookmark, 0, MAX_BOOKMARK_SIZE);

    state->next_request_id = 0;
    state->response_counter = 0;

    compile_RUN(&state->run, 0);
    compile_RUN(&state->begin, 0);
    BoltValue_format_as_String(state->begin.statement, "BEGIN", 5);
    compile_RUN(&state->commit, 0);
    BoltValue_format_as_String(state->commit.statement, "COMMIT", 6);
    compile_RUN(&state->rollback, 0);
    BoltValue_format_as_String(state->rollback.statement, "ROLLBACK", 8);

    state->discard_request = BoltMessage_create(DISCARD_ALL, 0);

    state->pull_request = BoltMessage_create(PULL_ALL, 0);

    state->reset_request = BoltMessage_create(RESET, 0);

    state->data_type = BOLT_V1_RECORD;
    state->data = BoltValue_create();
    return state;
}

void BoltProtocolV1_destroy_state(struct BoltProtocolV1State* state)
{
    if (state==NULL) return;

    BoltBuffer_destroy(state->tx_buffer);
    BoltBuffer_destroy(state->rx_buffer);

    BoltMessage_destroy(state->run.request);
    BoltMessage_destroy(state->begin.request);
    BoltMessage_destroy(state->commit.request);
    BoltMessage_destroy(state->rollback.request);

    BoltMessage_destroy(state->discard_request);
    BoltMessage_destroy(state->pull_request);

    BoltMessage_destroy(state->reset_request);

    BoltMem_deallocate(state->server, MAX_SERVER_SIZE);
    if (state->failure_data!=NULL) {
        BoltValue_destroy(state->failure_data);
    }
    BoltValue_destroy(state->result_field_names);
    BoltValue_destroy(state->result_metadata);
    BoltMem_deallocate(state->last_bookmark, MAX_BOOKMARK_SIZE);

    BoltValue_destroy(state->data);

    BoltMem_deallocate(state, sizeof(struct BoltProtocolV1State));
}

struct BoltProtocolV1State* BoltProtocolV1_state(struct BoltConnection* connection)
{
    return (struct BoltProtocolV1State*) (connection->protocol_state);
}

enum BoltProtocolV1Type marker_type(uint8_t marker)
{
    if (marker<0x80 || (marker>=0xC8 && marker<=0xCB) || marker>=0xF0) {
        return BOLT_V1_INTEGER;
    }
    if ((marker>=0x80 && marker<=0x8F) || (marker>=0xD0 && marker<=0xD2)) {
        return BOLT_V1_STRING;
    }
    if ((marker>=0x90 && marker<=0x9F) || (marker>=0xD4 && marker<=0xD6)) {
        return BOLT_V1_LIST;
    }
    if ((marker>=0xA0 && marker<=0xAF) || (marker>=0xD8 && marker<=0xDA)) {
        return BOLT_V1_MAP;
    }
    if ((marker>=0xB0 && marker<=0xBF) || (marker>=0xDC && marker<=0xDD)) {
        return BOLT_V1_STRUCTURE;
    }
    switch (marker) {
    case 0xC0:
        return BOLT_V1_NULL;
    case 0xC1:
        return BOLT_V1_FLOAT;
    case 0xC2:
    case 0xC3:
        return BOLT_V1_BOOLEAN;
    case 0xCC:
    case 0xCD:
    case 0xCE:
        return BOLT_V1_BYTES;
    default:
        return BOLT_V1_RESERVED;
    }
}

int load(struct BoltBuffer* buffer, struct BoltValue* value);

/**
 * Copy request data from buffer 1 to buffer 0, also adding chunks.
 *
 * @param connection
 */
void enqueue(struct BoltConnection* connection);

int load_null(struct BoltBuffer* buffer)
{
    BoltBuffer_load_u8(buffer, 0xC0);
    return 0;
}

int load_boolean(struct BoltBuffer* buffer, int value)
{
    BoltBuffer_load_u8(buffer, (value==0) ? (uint8_t) (0xC2) : (uint8_t) (0xC3));
    return 0;
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
    return 0;
}

int load_float(struct BoltBuffer* buffer, double value)
{
    BoltBuffer_load_u8(buffer, 0xC1);
    BoltBuffer_load_f64be(buffer, value);
    return 0;
}

int load_bytes(struct BoltBuffer* buffer, const char* string, int32_t size)
{
    if (size<0) {
        return -1;
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
    return 0;
}

int load_string_header(struct BoltBuffer* buffer, int32_t size)
{
    if (size<0) {
        return -1;
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
    return 0;
}

int load_string(struct BoltBuffer* buffer, const char* string, int32_t size)
{
    int status = load_string_header(buffer, size);
    if (status<0) return status;
    BoltBuffer_load(buffer, string, size);
    return 0;
}

int load_list_header(struct BoltBuffer* buffer, int32_t size)
{
    if (size<0) {
        return -1;
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
    return 0;
}

int load_map_header(struct BoltBuffer* buffer, int32_t size)
{
    if (size<0) {
        return -1;
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
    return 0;
}

int load_structure_header(struct BoltBuffer* buffer, int16_t code, int8_t size)
{
    if (code<0 || size<0 || size>=0x10) {
        return -1;
    }
    BoltBuffer_load_u8(buffer, (uint8_t) (0xB0+size));
    BoltBuffer_load_i8(buffer, (int8_t) (code));
    return 0;
}

int load_message(struct BoltConnection* connection, struct BoltMessage* message)
{
    struct BoltProtocolV1State* state = BoltProtocolV1_state(connection);
    TRY(load_structure_header(state->tx_buffer, message->code, (int8_t) (message->fields->size)));
    for (int32_t i = 0; i<message->fields->size; i++) {
        TRY(load(state->tx_buffer, BoltList_value(message->fields, i)));
    }
    enqueue(connection);
    return 0;
}

int BoltProtocolV1_load_message(struct BoltConnection* connection, struct BoltMessage* message)
{
    struct BoltProtocolV1State* state = BoltProtocolV1_state(connection);
    BoltLog_message("C", state->next_request_id, message->code, message->fields, connection->protocol_version);
    return load_message(connection, message);
}

int BoltProtocolV1_load_message_quietly(struct BoltConnection* connection, struct BoltMessage* message)
{
    return load_message(connection, message);
}

int load(struct BoltBuffer* buffer, struct BoltValue* value)
{
    switch (BoltValue_type(value)) {
    case BOLT_NULL:
        return load_null(buffer);
    case BOLT_LIST: {
        TRY(load_list_header(buffer, value->size));
        for (int32_t i = 0; i<value->size; i++) {
            TRY(load(buffer, BoltList_value(value, i)));
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
                TRY(load(buffer, BoltDictionary_value(value, i)));
            }
        }
        return 0;
    }
    case BOLT_INTEGER:
        return load_integer(buffer, BoltInteger_get(value));
    case BOLT_FLOAT:
        return load_float(buffer, BoltFloat_get(value));
    case BOLT_STRUCTURE: {
        TRY(load_structure_header(buffer, BoltStructure_code(value), (int8_t) value->size));
        for (int32_t i = 0; i<value->size; i++) {
            TRY(load(buffer, BoltStructure_value(value, i)));
        }
        return 0;
    }
    default:
        // TODO:  TYPE_NOT_SUPPORTED (vs TYPE_NOT_IMPLEMENTED)
        assert(0);
        return -1;
    }
}

/**
 * Copy request data from buffer 1 to buffer 0, also adding chunks.
 *
 * @param connection
 * @return request ID
 */
void enqueue(struct BoltConnection* connection)
{
    struct BoltProtocolV1State* state = BoltProtocolV1_state(connection);

    // loop through data, generate several chunks if it's larger than max chunk size
    int total_size = BoltBuffer_unloadable(state->tx_buffer);
    int total_remaining = total_size;
    char header[2];
    while (total_remaining > 0) {
        int current_size = total_remaining>BOLT_MAX_CHUNK_SIZE?BOLT_MAX_CHUNK_SIZE:total_remaining;
        header[0] = (char) (current_size >> 8);
        header[1] = (char) (current_size);
        BoltBuffer_load(connection->tx_buffer, &header[0], sizeof(header));
        BoltBuffer_load(connection->tx_buffer, BoltBuffer_unload_pointer(state->tx_buffer, current_size), current_size);
        total_remaining -= current_size;
    }

    header[0] = (char) (0);
    header[1] = (char) (0);
    BoltBuffer_load(connection->tx_buffer, &header[0], sizeof(header));
    BoltBuffer_compact(state->tx_buffer);
    state->next_request_id += 1;
}

int unload(struct BoltConnection* connection, struct BoltValue* value);

int unload_null(struct BoltConnection* connection, struct BoltValue* value)
{
    struct BoltProtocolV1State* state = BoltProtocolV1_state(connection);
    uint8_t marker;
    BoltBuffer_unload_u8(state->rx_buffer, &marker);
    if (marker==0xC0) {
        BoltValue_format_as_Null(value);
    }
    else {
        return -1;  // BOLT_ERROR_WRONG_TYPE
    }
    return 0;
}

int unload_boolean(struct BoltConnection* connection, struct BoltValue* value)
{
    struct BoltProtocolV1State* state = BoltProtocolV1_state(connection);
    uint8_t marker;
    BoltBuffer_unload_u8(state->rx_buffer, &marker);
    if (marker==0xC3) {
        BoltValue_format_as_Boolean(value, 1);
    }
    else if (marker==0xC2) {
        BoltValue_format_as_Boolean(value, 0);
    }
    else {
        return -1;  // BOLT_ERROR_WRONG_TYPE
    }
    return 0;
}

int unload_integer(struct BoltConnection* connection, struct BoltValue* value)
{
    struct BoltProtocolV1State* state = BoltProtocolV1_state(connection);
    uint8_t marker;
    BoltBuffer_unload_u8(state->rx_buffer, &marker);
    if (marker<0x80) {
        BoltValue_format_as_Integer(value, marker);
    }
    else if (marker>=0xF0) {
        BoltValue_format_as_Integer(value, marker-0x100);
    }
    else if (marker==0xC8) {
        int8_t x;
        BoltBuffer_unload_i8(state->rx_buffer, &x);
        BoltValue_format_as_Integer(value, x);
    }
    else if (marker==0xC9) {
        int16_t x;
        BoltBuffer_unload_i16be(state->rx_buffer, &x);
        BoltValue_format_as_Integer(value, x);
    }
    else if (marker==0xCA) {
        int32_t x;
        BoltBuffer_unload_i32be(state->rx_buffer, &x);
        BoltValue_format_as_Integer(value, x);
    }
    else if (marker==0xCB) {
        int64_t x;
        BoltBuffer_unload_i64be(state->rx_buffer, &x);
        BoltValue_format_as_Integer(value, x);
    }
    else {
        return -1;  // BOLT_ERROR_WRONG_TYPE
    }
    return 0;
}

int unload_float(struct BoltConnection* connection, struct BoltValue* value)
{
    struct BoltProtocolV1State* state = BoltProtocolV1_state(connection);
    uint8_t marker;
    BoltBuffer_unload_u8(state->rx_buffer, &marker);
    if (marker==0xC1) {
        double x;
        BoltBuffer_unload_f64be(state->rx_buffer, &x);
        BoltValue_format_as_Float(value, x);
    }
    else {
        return -1;  // BOLT_ERROR_WRONG_TYPE
    }
    return 0;
}

int unload_string(struct BoltConnection* connection, struct BoltValue* value)
{
    struct BoltProtocolV1State* state = BoltProtocolV1_state(connection);
    uint8_t marker;
    BoltBuffer_unload_u8(state->rx_buffer, &marker);
    if (marker>=0x80 && marker<=0x8F) {
        int32_t size;
        size = marker & 0x0F;
        BoltValue_format_as_String(value, NULL, size);
        BoltBuffer_unload(state->rx_buffer, BoltString_get(value), size);
        return 0;
    }
    if (marker==0xD0) {
        uint8_t size;
        BoltBuffer_unload_u8(state->rx_buffer, &size);
        BoltValue_format_as_String(value, NULL, size);
        BoltBuffer_unload(state->rx_buffer, BoltString_get(value), size);
        return 0;
    }
    if (marker==0xD1) {
        uint16_t size;
        BoltBuffer_unload_u16be(state->rx_buffer, &size);
        BoltValue_format_as_String(value, NULL, size);
        BoltBuffer_unload(state->rx_buffer, BoltString_get(value), size);
        return 0;
    }
    if (marker==0xD2) {
        int32_t size;
        BoltBuffer_unload_i32be(state->rx_buffer, &size);
        BoltValue_format_as_String(value, NULL, size);
        BoltBuffer_unload(state->rx_buffer, BoltString_get(value), size);
        return 0;
    }
    BoltLog_error("bolt: Unknown marker: %d", marker);
    return -1;  // BOLT_ERROR_WRONG_TYPE
}

int unload_bytes(struct BoltConnection* connection, struct BoltValue* value)
{
    struct BoltProtocolV1State* state = BoltProtocolV1_state(connection);
    uint8_t marker;
    BoltBuffer_unload_u8(state->rx_buffer, &marker);
    if (marker==0xCC) {
        uint8_t size;
        BoltBuffer_unload_u8(state->rx_buffer, &size);
        BoltValue_format_as_Bytes(value, NULL, size);
        BoltBuffer_unload(state->rx_buffer, BoltBytes_get_all(value), size);
        return 0;
    }
    if (marker==0xCD) {
        uint16_t size;
        BoltBuffer_unload_u16be(state->rx_buffer, &size);
        BoltValue_format_as_Bytes(value, NULL, size);
        BoltBuffer_unload(state->rx_buffer, BoltBytes_get_all(value), size);
        return 0;
    }
    if (marker==0xCE) {
        int32_t size;
        BoltBuffer_unload_i32be(state->rx_buffer, &size);
        BoltValue_format_as_Bytes(value, NULL, size);
        BoltBuffer_unload(state->rx_buffer, BoltBytes_get_all(value), size);
        return 0;
    }
    BoltLog_error("bolt: Unknown marker: %d", marker);
    return -1;  // BOLT_ERROR_WRONG_TYPE
}

int unload_list(struct BoltConnection* connection, struct BoltValue* value)
{
    struct BoltProtocolV1State* state = BoltProtocolV1_state(connection);
    uint8_t marker;
    int32_t size;
    BoltBuffer_unload_u8(state->rx_buffer, &marker);
    if (marker>=0x90 && marker<=0x9F) {
        size = marker & 0x0F;
    }
    else if (marker==0xD4) {
        uint8_t size_;
        BoltBuffer_unload_u8(state->rx_buffer, &size_);
        size = size_;
    }
    else if (marker==0xD5) {
        uint16_t size_;
        BoltBuffer_unload_u16be(state->rx_buffer, &size_);
        size = size_;
    }
    else if (marker==0xD6) {
        int32_t size_;
        BoltBuffer_unload_i32be(state->rx_buffer, &size_);
        size = size_;
    }
    else {
        return -1;  // BOLT_ERROR_WRONG_TYPE
    }
    if (size<0) {
        return -1;  // invalid size
    }
    BoltValue_format_as_List(value, size);
    for (int i = 0; i<size; i++) {
        unload(connection, BoltList_value(value, i));
    }
    return size;
}

int unload_map(struct BoltConnection* connection, struct BoltValue* value)
{
    struct BoltProtocolV1State* state = BoltProtocolV1_state(connection);
    uint8_t marker;
    int32_t size;
    BoltBuffer_unload_u8(state->rx_buffer, &marker);
    if (marker>=0xA0 && marker<=0xAF) {
        size = marker & 0x0F;
    }
    else if (marker==0xD8) {
        uint8_t size_;
        BoltBuffer_unload_u8(state->rx_buffer, &size_);
        size = size_;
    }
    else if (marker==0xD9) {
        uint16_t size_;
        BoltBuffer_unload_u16be(state->rx_buffer, &size_);
        size = size_;
    }
    else if (marker==0xDA) {
        int32_t size_;
        BoltBuffer_unload_i32be(state->rx_buffer, &size_);
        size = size_;
    }
    else {
        return -1;  // BOLT_ERROR_WRONG_TYPE
    }
    if (size<0) {
        return -1;  // invalid size
    }
    BoltValue_format_as_Dictionary(value, size);
    for (int i = 0; i<size; i++) {
        unload(connection, BoltDictionary_key(value, i));
        unload(connection, BoltDictionary_value(value, i));
    }
    return size;
}

int unload_structure(struct BoltConnection* connection, struct BoltValue* value)
{
    struct BoltProtocolV1State* state = BoltProtocolV1_state(connection);
    uint8_t marker;
    int8_t code;
    int32_t size;
    BoltBuffer_unload_u8(state->rx_buffer, &marker);
    if (marker>=0xB0 && marker<=0xBF) {
        size = marker & 0x0F;
        BoltBuffer_unload_i8(state->rx_buffer, &code);
        BoltValue_format_as_Structure(value, code, size);
        for (int i = 0; i<size; i++) {
            unload(connection, BoltStructure_value(value, i));
        }
        return 0;
    }
    // TODO: bigger structures (that are never actually used)
    return -1;  // BOLT_ERROR_WRONG_TYPE
}

int unload(struct BoltConnection* connection, struct BoltValue* value)
{
    struct BoltProtocolV1State* state = BoltProtocolV1_state(connection);
    uint8_t marker;
    BoltBuffer_peek_u8(state->rx_buffer, &marker);
    switch (marker_type(marker)) {
    case BOLT_V1_NULL:
        return unload_null(connection, value);
    case BOLT_V1_BOOLEAN:
        return unload_boolean(connection, value);
    case BOLT_V1_INTEGER:
        return unload_integer(connection, value);
    case BOLT_V1_FLOAT:
        return unload_float(connection, value);
    case BOLT_V1_STRING:
        return unload_string(connection, value);
    case BOLT_V1_BYTES:
        return unload_bytes(connection, value);
    case BOLT_V1_LIST:
        return unload_list(connection, value);
    case BOLT_V1_MAP:
        return unload_map(connection, value);
    case BOLT_V1_STRUCTURE:
        return unload_structure(connection, value);
    default:
        BoltLog_error("bolt: Unknown marker: %d", marker);
        return -1;  // BOLT_UNSUPPORTED_MARKER
    }
}

void ensure_failure_data(struct BoltProtocolV1State* state)
{
    if (state->failure_data==NULL) {
        state->failure_data = BoltValue_create();
        BoltValue_format_as_Dictionary(state->failure_data, 2);
        BoltDictionary_set_key(state->failure_data, 0, "code", strlen("code"));
        BoltDictionary_set_key(state->failure_data, 1, "message", strlen("message"));
    }
}

void clear_failure_data(struct BoltProtocolV1State* state)
{
    if (state->failure_data!=NULL) {
        BoltValue_destroy(state->failure_data);
        state->failure_data = NULL;
    }
}

int BoltProtocolV1_fetch(struct BoltConnection* connection, bolt_request_t request_id)
{
    struct BoltProtocolV1State* state = BoltProtocolV1_state(connection);
    bolt_request_t response_id;
    do {
        char header[2];
        int fetched = BoltConnection_receive(connection, &header[0], 2);
        if (fetched==-1) {
            BoltLog_error("bolt: Could not fetch chunk header");
            return -1;
        }
        uint16_t chunk_size = char_to_uint16be(header);
        BoltBuffer_compact(state->rx_buffer);
        while (chunk_size!=0) {
            fetched = BoltConnection_receive(connection, BoltBuffer_load_pointer(state->rx_buffer, chunk_size),
                    chunk_size);
            if (fetched==-1) {
                BoltLog_error("bolt: Could not fetch chunk data");
                return -1;
            }
            fetched = BoltConnection_receive(connection, &header[0], 2);
            if (fetched==-1) {
                BoltLog_error("bolt: Could not fetch chunk header");
                return -1;
            }
            chunk_size = char_to_uint16be(header);
        }
        response_id = state->response_counter;
        BoltProtocolV1_unload(connection);
        if (state->data_type!=BOLT_V1_RECORD) {
            state->response_counter += 1;

            // Clean existing metadata
            BoltValue_format_as_Dictionary(state->result_metadata, 0);

            if (state->data->size>=1) {
                BoltProtocolV1_extract_metadata(connection, BoltList_value(state->data, 0));
            }
        }
    }
    while (response_id!=request_id);

    if (state->data_type!=BOLT_V1_RECORD) {
        return 0;
    }

    return 1;
}

int BoltProtocolV1_unload(struct BoltConnection* connection)
{
    struct BoltProtocolV1State* state = BoltProtocolV1_state(connection);
    if (BoltBuffer_unloadable(state->rx_buffer)==0) {
        return 0;
    }

    uint8_t marker;
    BoltBuffer_unload_u8(state->rx_buffer, &marker);
    if (marker_type(marker)!=BOLT_V1_STRUCTURE) {
        return -1;
    }

    uint8_t code;
    BoltBuffer_unload_u8(state->rx_buffer, &code);
    state->data_type = code;

    int32_t size = marker & 0x0F;
    BoltValue_format_as_List(state->data, size);
    for (int i = 0; i<size; i++) {
        unload(connection, BoltList_value(state->data, i));
    }
    if (code==BOLT_V1_RECORD) {
        if (state->record_counter<MAX_LOGGED_RECORDS) {
            BoltLog_message("S", state->response_counter, code, state->data, connection->protocol_version);
        }
        state->record_counter += 1;
    }
    else {
        if (state->record_counter>MAX_LOGGED_RECORDS) {
            BoltLog_info("bolt: S[%d]: Received %llu more records", state->response_counter,
                    state->record_counter-MAX_LOGGED_RECORDS);
        }
        state->record_counter = 0;
        BoltLog_message("S", state->response_counter, code, state->data, connection->protocol_version);
    }
    return 1;
}

const char* BoltProtocolV1_structure_name(int16_t code)
{
    switch (code) {
    case 'N':
        return "Node";
    case 'R':
        return "Relationship";
    case 'r':
        return "UnboundRelationship";
    case 'P':
        return "Path";
    default:
        return "?";
    }
}

const char* BoltProtocolV1_message_name(int16_t code)
{
    switch (code) {
    case 0x01:
        return "INIT";
    case 0x0E:
        return "ACK_FAILURE";
    case 0x0F:
        return "RESET";
    case 0x10:
        return "RUN";
    case 0x2F:
        return "DISCARD_ALL";
    case 0x3F:
        return "PULL_ALL";
    case 0x70:
        return "SUCCESS";
    case 0x71:
        return "RECORD";
    case 0x7E:
        return "IGNORED";
    case 0x7F:
        return "FAILURE";
    default:
        return NULL;
    }
}

int BoltProtocolV1_init(struct BoltConnection* connection, const char* user_agent, const struct BoltValue* auth_token)
{
    struct BoltMessage* init = BoltMessage_create(INIT, 2);
    struct BoltProtocolV1State* state = BoltProtocolV1_state(connection);
    BoltProtocolV1_compile_INIT(init, user_agent, auth_token, 1);
    BoltLog_message("C", state->next_request_id, init->code, init->fields, connection->protocol_version);
    BoltProtocolV1_compile_INIT(init, user_agent, auth_token, 0);
    BoltProtocolV1_load_message_quietly(connection, init);
    bolt_request_t init_request = BoltConnection_last_request(connection);
    BoltMessage_destroy(init);
    TRY(BoltConnection_send(connection));
    TRY(BoltConnection_fetch_summary(connection, init_request));
    return state->data_type;
}

void BoltProtocolV1_clear_failure(struct BoltConnection* connection)
{
    struct BoltProtocolV1State* state = BoltProtocolV1_state(connection);
    clear_failure_data(state);
}

void BoltProtocolV1_extract_metadata(struct BoltConnection* connection, struct BoltValue* metadata)
{
    struct BoltProtocolV1State* state = BoltProtocolV1_state(connection);

    switch (BoltValue_type(metadata)) {
    case BOLT_DICTIONARY: {
        for (int32_t i = 0; i<metadata->size; i++) {
            struct BoltValue* key = BoltDictionary_key(metadata, i);

            if (BoltString_equals(key, "bookmark")) {
                struct BoltValue* value = BoltDictionary_value(metadata, i);
                switch (BoltValue_type(value)) {
                case BOLT_STRING: {
                    memset(state->last_bookmark, 0, MAX_BOOKMARK_SIZE);
                    memcpy(state->last_bookmark, BoltString_get(value), (size_t) (value->size));
                    BoltLog_info("bolt: <SET last_bookmark=\"%s\">", state->last_bookmark);
                    break;
                }
                default:
                    break;
                }
            }
            else if (BoltString_equals(key, "fields")) {
                struct BoltValue* value = BoltDictionary_value(metadata, i);
                switch (BoltValue_type(value)) {
                case BOLT_LIST: {
                    struct BoltValue* target_value = state->result_field_names;
                    BoltValue_format_as_List(target_value, value->size);
                    for (int j = 0; j<value->size; j++) {
                        struct BoltValue* source_value = BoltList_value(value, j);
                        switch (BoltValue_type(source_value)) {
                        case BOLT_STRING:
                            BoltValue_format_as_String(BoltList_value(target_value, j),
                                    BoltString_get(source_value), source_value->size);
                            break;
                        default:
                            BoltValue_format_as_Null(BoltList_value(target_value, j));
                        }
                    }
                    BoltLog_value(target_value, 1, "<SET result_field_names=", ">");
                    break;
                }
                default:
                    break;
                }
            }
            else if (BoltString_equals(key, "server")) {
                struct BoltValue* value = BoltDictionary_value(metadata, i);
                switch (BoltValue_type(value)) {
                case BOLT_STRING: {
                    memset(state->server, 0, MAX_SERVER_SIZE);
                    memcpy(state->server, BoltString_get(value), (size_t) (value->size));
                    BoltLog_info("bolt: <SET server=\"%s\">", state->server);
                    break;
                }
                default:
                    break;
                }
            }
            else if (BoltString_equals(key, "code") && state->data_type==BOLT_V1_FAILURE) {
                struct BoltValue* value = BoltDictionary_value(metadata, i);
                switch (BoltValue_type(value)) {
                case BOLT_STRING: {
                    ensure_failure_data(state);

                    struct BoltValue* target_value = BoltDictionary_value(state->failure_data, 0);
                    BoltValue_format_as_String(target_value, BoltString_get(value), value->size);
                    BoltLog_value(target_value, 1, "bolt: <FAILURE code=\"", "\">");
                    break;
                }
                default:
                    break;
                }
            }
            else if (BoltString_equals(key, "message") && state->data_type==BOLT_V1_FAILURE) {
                struct BoltValue* value = BoltDictionary_value(metadata, i);
                switch (BoltValue_type(value)) {
                case BOLT_STRING: {
                    ensure_failure_data(state);

                    struct BoltValue* target_value = BoltDictionary_value(state->failure_data, 1);
                    BoltValue_format_as_String(target_value, BoltString_get(value), value->size);
                    BoltLog_value(target_value, 1, "bolt: <FAILURE message=\"", "\">");
                    break;
                }
                default:
                    break;
                }
            }
            else {
                struct BoltValue* source_key = BoltDictionary_key(metadata, i);
                struct BoltValue* source_value = BoltDictionary_value(metadata, i);

                // increase length
                int32_t index = state->result_metadata->size;
                BoltValue_format_as_Dictionary(state->result_metadata, index+1);

                struct BoltValue* dest_key = BoltDictionary_key(state->result_metadata, index);
                struct BoltValue* dest_value = BoltDictionary_value(state->result_metadata, index);

                BoltValue_copy(dest_key, source_key);
                BoltValue_copy(dest_value, source_value);
            }
        }
        break;
    }
    default:
        break;
    }
}

int BoltProtocolV1_set_cypher_template(struct BoltConnection* connection, const char* statement, size_t size)
{
    if (size<=INT32_MAX) {
        struct BoltProtocolV1State* state = BoltProtocolV1_state(connection);
        BoltValue_format_as_String(state->run.statement, statement, (int32_t) (size));
        return 0;
    }
    return -1;
}

int BoltProtocolV1_set_n_cypher_parameters(struct BoltConnection* connection, int32_t size)
{
    struct BoltProtocolV1State* state = BoltProtocolV1_state(connection);
    BoltValue_format_as_Dictionary(state->run.parameters, size);
    return 0;
}

int BoltProtocolV1_set_cypher_parameter_key(struct BoltConnection* connection, int32_t index, const char* key,
        size_t key_size)
{
    struct BoltProtocolV1State* state = BoltProtocolV1_state(connection);
    return BoltDictionary_set_key(state->run.parameters, index, key, key_size);
}

struct BoltValue* BoltProtocolV1_cypher_parameter_value(struct BoltConnection* connection, int32_t index)
{
    struct BoltProtocolV1State* state = BoltProtocolV1_state(connection);
    return BoltDictionary_value(state->run.parameters, index);
}

int BoltProtocolV1_load_bookmark(struct BoltConnection* connection, const char* bookmark)
{
    if (bookmark==NULL) {
        return 0;
    }
    struct BoltProtocolV1State* state = BoltProtocolV1_state(connection);
    struct BoltValue* bookmarks;
    if (state->begin.parameters->size==0) {
        BoltValue_format_as_Dictionary(state->begin.parameters, 1);
        BoltDictionary_set_key(state->begin.parameters, 0, "bookmarks", 9);
        bookmarks = BoltDictionary_value(state->begin.parameters, 0);
        BoltValue_format_as_List(bookmarks, 0);
    }
    else {
        bookmarks = BoltDictionary_value(state->begin.parameters, 0);
    }
    int32_t n_bookmarks = bookmarks->size;
    BoltList_resize(bookmarks, n_bookmarks+1);
    size_t bookmark_size = strlen(bookmark);
    if (bookmark_size>INT32_MAX) {
        return -1;
    }
    BoltValue_format_as_String(BoltList_value(bookmarks, n_bookmarks), bookmark, (int32_t) (bookmark_size));
    return 1;
}

int BoltProtocolV1_load_begin_request(struct BoltConnection* connection)
{
    struct BoltProtocolV1State* state = BoltProtocolV1_state(connection);
    BoltProtocolV1_load_message(connection, state->begin.request);
    BoltValue_format_as_Dictionary(state->begin.parameters, 0);
    BoltProtocolV1_load_message(connection, state->discard_request);
    return 0;
}

int BoltProtocolV1_load_commit_request(struct BoltConnection* connection)
{
    struct BoltProtocolV1State* state = BoltProtocolV1_state(connection);
    BoltProtocolV1_load_message(connection, state->commit.request);
    BoltProtocolV1_load_message(connection, state->discard_request);
    return 0;
}

int BoltProtocolV1_load_rollback_request(struct BoltConnection* connection)
{
    struct BoltProtocolV1State* state = BoltProtocolV1_state(connection);
    BoltProtocolV1_load_message(connection, state->rollback.request);
    BoltProtocolV1_load_message(connection, state->discard_request);
    return 0;
}

int BoltProtocolV1_load_run_request(struct BoltConnection* connection)
{
    struct BoltProtocolV1State* state = BoltProtocolV1_state(connection);
    BoltProtocolV1_load_message(connection, state->run.request);
    return 0;
}

int BoltProtocolV1_load_pull_request(struct BoltConnection* connection, int32_t n)
{
    if (n>=0) {
        return -1;
    }
    else {
        struct BoltProtocolV1State* state = BoltProtocolV1_state(connection);
        BoltProtocolV1_load_message(connection, state->pull_request);
        return 0;
    }
}

int BoltProtocolV1_load_reset_request(struct BoltConnection* connection)
{
    struct BoltProtocolV1State* state = BoltProtocolV1_state(connection);
    BoltProtocolV1_load_message(connection, state->reset_request);
    BoltProtocolV1_clear_failure(connection);
    return 0;
}

struct BoltValue* BoltProtocolV1_result_fields(struct BoltConnection* connection)
{
    struct BoltProtocolV1State* state = BoltProtocolV1_state(connection);
    switch (BoltValue_type(state->result_field_names)) {
    case BOLT_LIST:
        return state->result_field_names;
    default:
        return NULL;
    }
}

struct BoltValue* BoltProtocolV1_result_metadata(struct BoltConnection* connection)
{
    struct BoltProtocolV1State* state = BoltProtocolV1_state(connection);
    switch (BoltValue_type(state->result_metadata)) {
    case BOLT_DICTIONARY:
        return state->result_metadata;
    default:
        return NULL;
    }
}
