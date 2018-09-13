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


#include <assert.h>
#include <memory.h>
#include <string.h>
#include <stdlib.h>
#include <bolt/connections.h>

#include "bolt/buffering.h"
#include "bolt/logging.h"
#include "bolt/mem.h"
#include "packstream.h"
#include "v1.h"

#define INIT        0x01
#define ACK_FAILURE 0x0E
#define RESET       0x0F
#define RUN         0x10
#define DISCARD_ALL 0x2F
#define PULL_ALL    0x3F

#define NODE    'N'
#define RELATIONSHIP 'R'
#define UNBOUND_RELATIONSHIP 'r'
#define PATH 'P'

#define INITIAL_TX_BUFFER_SIZE 8192
#define INITIAL_RX_BUFFER_SIZE 8192

#define MAX_BOOKMARK_SIZE 40
#define MAX_SERVER_SIZE 200

#define MAX_LOGGED_RECORDS 3

#define char_to_uint16be(array) ((uint8_t)(header[0]) << 8) | (uint8_t)(header[1]);

#define TRY(code) { int status_try = (code); if (status_try != BOLT_SUCCESS) { return status_try; } }

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

    return BOLT_SUCCESS;
}

void compile_RUN(struct _run_request* run, int32_t n_parameters)
{
    run->request = BoltMessage_create(RUN, 2);
    run->statement = BoltList_value(run->request->fields, 0);
    run->parameters = BoltList_value(run->request->fields, 1);
    BoltValue_format_as_Dictionary(run->parameters, n_parameters);
}

int BoltProtocolV1_check_readable_struct_signature(int16_t signature)
{
    switch (signature) {
    case BOLT_V1_SUCCESS:
    case BOLT_V1_FAILURE:
    case BOLT_V1_IGNORED:
    case BOLT_V1_RECORD:
    case NODE:
    case RELATIONSHIP:
    case UNBOUND_RELATIONSHIP:
    case PATH:
        return 1;
    }

    return 0;
}

int BoltProtocolV1_check_writable_struct_signature(int16_t signature)
{
    switch (signature) {
    case INIT:
    case ACK_FAILURE:
    case RESET:
    case RUN:
    case DISCARD_ALL:
    case PULL_ALL:
        return 1;
    }

    return 0;
}

struct BoltProtocolV1State* BoltProtocolV1_create_state()
{
    struct BoltProtocolV1State* state = BoltMem_allocate(sizeof(struct BoltProtocolV1State));

    state->check_readable_struct = &BoltProtocolV1_check_readable_struct_signature;
    state->check_writable_struct = &BoltProtocolV1_check_writable_struct_signature;

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

/**
 * Copy request data from buffer 1 to buffer 0, also adding chunks.
 *
 * @param connection
 */
void enqueue(struct BoltConnection* connection);

int load_message(struct BoltConnection* connection, struct BoltMessage* message)
{
    struct BoltProtocolV1State* state = BoltProtocolV1_state(connection);
    if (state->check_writable_struct(message->code)) {
        TRY(load_structure_header(state->tx_buffer, message->code, (int8_t) (message->fields->size)));
        for (int32_t i = 0; i<message->fields->size; i++) {
            TRY(load(state->check_writable_struct, state->tx_buffer, BoltList_value(message->fields, i),
                    connection->log));
        }
        enqueue(connection);
        return BOLT_SUCCESS;
    }
    return BOLT_PROTOCOL_UNSUPPORTED_TYPE;
}

int BoltProtocolV1_load_message(struct BoltConnection* connection, struct BoltMessage* message, int quiet)
{
    if (!quiet) {
        struct BoltProtocolV1State* state = BoltProtocolV1_state(connection);
        BoltLog_message(connection->log, "C", state->next_request_id, message->code, message->fields,
                connection->protocol_version);
    }

    struct BoltProtocolV1State* state = BoltProtocolV1_state(connection);
    int prev_cursor = state->tx_buffer->cursor;
    int prev_extent = state->tx_buffer->extent;
    int status = load_message(connection, message);
    if (status!=BOLT_SUCCESS) {
        // Reset buffer to its previous state
        state->tx_buffer->cursor = prev_cursor;
        state->tx_buffer->extent = prev_extent;
    }
    return status;
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
    while (total_remaining>0) {
        int current_size = total_remaining>BOLT_MAX_CHUNK_SIZE ? BOLT_MAX_CHUNK_SIZE : total_remaining;
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
        int status = BoltConnection_receive(connection, &header[0], 2);
        if (status!=BOLT_SUCCESS) {
            BoltLog_error(connection->log, "Could not fetch chunk header");
            return -1;
        }
        uint16_t chunk_size = char_to_uint16be(header);
        BoltBuffer_compact(state->rx_buffer);
        while (chunk_size!=0) {
            status = BoltConnection_receive(connection, BoltBuffer_load_pointer(state->rx_buffer, chunk_size),
                    chunk_size);
            if (status!=BOLT_SUCCESS) {
                BoltLog_error(connection->log, "Could not fetch chunk data");
                return -1;
            }
            status = BoltConnection_receive(connection, &header[0], 2);
            if (status!=BOLT_SUCCESS) {
                BoltLog_error(connection->log, "Could not fetch chunk header");
                return -1;
            }
            chunk_size = char_to_uint16be(header);
        }
        response_id = state->response_counter;
        TRY(BoltProtocolV1_unload(connection));
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
    TRY(BoltBuffer_unload_u8(state->rx_buffer, &marker));
    if (marker_type(marker)!=PACKSTREAM_STRUCTURE) {
        return BOLT_PROTOCOL_VIOLATION;
    }

    uint8_t code;
    TRY(BoltBuffer_unload_u8(state->rx_buffer, &code));
    state->data_type = code;

    int32_t size = marker & 0x0F;
    BoltValue_format_as_List(state->data, size);
    for (int i = 0; i<size; i++) {
        TRY(unload(state->check_readable_struct, state->rx_buffer, BoltList_value(state->data, i),
                connection->log));
    }
    if (code==BOLT_V1_RECORD) {
        if (state->record_counter<MAX_LOGGED_RECORDS) {
            BoltLog_message(connection->log, "S", state->response_counter, code, state->data,
                    connection->protocol_version);
        }
        state->record_counter += 1;
    }
    else {
        if (state->record_counter>MAX_LOGGED_RECORDS) {
            BoltLog_info(connection->log, "S[%d]: Received %llu more records", state->response_counter,
                    state->record_counter-MAX_LOGGED_RECORDS);
        }
        state->record_counter = 0;
        BoltLog_message(connection->log, "S", state->response_counter, code, state->data, connection->protocol_version);
    }
    return BOLT_SUCCESS;
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
    TRY(BoltProtocolV1_compile_INIT(init, user_agent, auth_token, 1));
    BoltLog_message(connection->log, "C", state->next_request_id, init->code, init->fields, connection->protocol_version);
    TRY(BoltProtocolV1_compile_INIT(init, user_agent, auth_token, 0));
    TRY(BoltProtocolV1_load_message(connection, init, 1));
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
                    BoltLog_info(connection->log, "<SET last_bookmark=\"%s\">", state->last_bookmark);
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
                    BoltLog_value(connection->log, "<SET result_field_names=%s>", target_value,
                            connection->protocol_version);
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
                    BoltLog_info(connection->log, "<SET server=\"%s\">", state->server);
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

                    BoltLog_value(connection->log, "<FAILURE code=\"%s\">", target_value,
                            connection->protocol_version);

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

                    BoltLog_value(connection->log, "<FAILURE message=\"%s\">", target_value,
                            connection->protocol_version);

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
        return BOLT_SUCCESS;
    }
    return BOLT_PROTOCOL_VIOLATION;
}

int BoltProtocolV1_set_n_cypher_parameters(struct BoltConnection* connection, int32_t size)
{
    struct BoltProtocolV1State* state = BoltProtocolV1_state(connection);
    BoltValue_format_as_Dictionary(state->run.parameters, size);
    return BOLT_SUCCESS;
}

int BoltProtocolV1_set_cypher_parameter_key(struct BoltConnection* connection, int32_t index, const char* key,
        size_t key_size)
{
    struct BoltProtocolV1State* state = BoltProtocolV1_state(connection);
    if (BoltDictionary_set_key(state->run.parameters, index, key, key_size)) {
        return BOLT_PROTOCOL_VIOLATION;
    }
    return BOLT_SUCCESS;
}

struct BoltValue* BoltProtocolV1_cypher_parameter_value(struct BoltConnection* connection, int32_t index)
{
    struct BoltProtocolV1State* state = BoltProtocolV1_state(connection);
    return BoltDictionary_value(state->run.parameters, index);
}

int BoltProtocolV1_load_bookmark(struct BoltConnection* connection, const char* bookmark)
{
    if (bookmark==NULL) {
        return BOLT_SUCCESS;
    }
    struct BoltProtocolV1State* state = BoltProtocolV1_state(connection);
    struct BoltValue* bookmarks;
    if (state->begin.parameters->size==0) {
        BoltValue_format_as_Dictionary(state->begin.parameters, 1);
        if (BoltDictionary_set_key(state->begin.parameters, 0, "bookmarks", 9)) {
            return BOLT_PROTOCOL_VIOLATION;
        }
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
        return BOLT_PROTOCOL_VIOLATION;
    }
    BoltValue_format_as_String(BoltList_value(bookmarks, n_bookmarks), bookmark, (int32_t) (bookmark_size));
    return BOLT_SUCCESS;
}

int BoltProtocolV1_load_begin_request(struct BoltConnection* connection)
{
    struct BoltProtocolV1State* state = BoltProtocolV1_state(connection);
    TRY(BoltProtocolV1_load_message(connection, state->begin.request, 0));
    BoltValue_format_as_Dictionary(state->begin.parameters, 0);
    TRY(BoltProtocolV1_load_message(connection, state->discard_request, 0));
    return BOLT_SUCCESS;
}

int BoltProtocolV1_load_commit_request(struct BoltConnection* connection)
{
    struct BoltProtocolV1State* state = BoltProtocolV1_state(connection);
    TRY(BoltProtocolV1_load_message(connection, state->commit.request, 0));
    TRY(BoltProtocolV1_load_message(connection, state->discard_request, 0));
    return BOLT_SUCCESS;
}

int BoltProtocolV1_load_rollback_request(struct BoltConnection* connection)
{
    struct BoltProtocolV1State* state = BoltProtocolV1_state(connection);
    TRY(BoltProtocolV1_load_message(connection, state->rollback.request, 0));
    TRY(BoltProtocolV1_load_message(connection, state->discard_request, 0));
    return BOLT_SUCCESS;
}

int BoltProtocolV1_load_run_request(struct BoltConnection* connection)
{
    struct BoltProtocolV1State* state = BoltProtocolV1_state(connection);
    TRY(BoltProtocolV1_load_message(connection, state->run.request, 0));
    return BOLT_SUCCESS;
}

int BoltProtocolV1_load_pull_request(struct BoltConnection* connection, int32_t n)
{
    if (n>=0) {
        return BOLT_PROTOCOL_VIOLATION;
    }
    else {
        struct BoltProtocolV1State* state = BoltProtocolV1_state(connection);
        TRY(BoltProtocolV1_load_message(connection, state->pull_request, 0));
        return BOLT_SUCCESS;
    }
}

int BoltProtocolV1_load_reset_request(struct BoltConnection* connection)
{
    struct BoltProtocolV1State* state = BoltProtocolV1_state(connection);
    TRY(BoltProtocolV1_load_message(connection, state->reset_request, 0));
    BoltProtocolV1_clear_failure(connection);
    return BOLT_SUCCESS;
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
