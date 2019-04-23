/*
 * Copyright (c) 2002-2019 "Neo4j,"
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
#include "connection-private.h"
#include "log-private.h"
#include "mem.h"
#include "protocol.h"
#include "v1.h"
#include "values-private.h"

#define BOOKMARKS_KEY "bookmarks"
#define BOOKMARKS_KEY_SIZE 9
#define BOOKMARK_KEY "bookmark"
#define BOOKMARK_KEY_SIZE 8
#define FIELDS_KEY "fields"
#define FIELDS_KEY_SIZE 6
#define SERVER_KEY "server"
#define SERVER_KEY_SIZE 6
#define FAILURE_CODE_KEY "code"
#define FAILURE_CODE_KEY_SIZE 4
#define FAILURE_MESSAGE_KEY "message"
#define FAILURE_MESSAGE_KEY_SIZE 7

#define INITIAL_TX_BUFFER_SIZE 8192
#define INITIAL_RX_BUFFER_SIZE 8192

#define MAX_BOOKMARK_SIZE 40
#define MAX_SERVER_SIZE 200

#define MAX_LOGGED_RECORDS 3

#define char_to_uint16be(array) ((uint8_t)(header[0]) << 8) | (uint8_t)(header[1]);

#define TRY(code) { int status_try = (code); if (status_try != BOLT_SUCCESS) { return status_try; } }

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

struct BoltMessage* create_run_message(const char* statement, size_t statement_size, int32_t n_parameters)
{
    struct BoltMessage* message = BoltMessage_create(BOLT_V1_RUN, 2);
    BoltValue_format_as_String(BoltMessage_param(message, 0), statement, (int32_t) statement_size);
    BoltValue_format_as_Dictionary(BoltMessage_param(message, 1), n_parameters);
    return message;
}

int BoltProtocolV1_check_readable_struct_signature(int16_t signature)
{
    switch (signature) {
    case BOLT_V1_SUCCESS:
    case BOLT_V1_FAILURE:
    case BOLT_V1_IGNORED:
    case BOLT_V1_RECORD:
    case BOLT_V1_NODE:
    case BOLT_V1_RELATIONSHIP:
    case BOLT_V1_UNBOUND_RELATIONSHIP:
    case BOLT_V1_PATH:
        return 1;
    }

    return 0;
}

int BoltProtocolV1_check_writable_struct_signature(int16_t signature)
{
    switch (signature) {
    case BOLT_V1_INIT:
    case BOLT_V1_ACK_FAILURE:
    case BOLT_V1_RESET:
    case BOLT_V1_RUN:
    case BOLT_V1_DISCARD_ALL:
    case BOLT_V1_PULL_ALL:
        return 1;
    }

    return 0;
}

const char* BoltProtocolV1_structure_name(int16_t code)
{
    switch (code) {
    case BOLT_V1_NODE:
        return "Node";
    case BOLT_V1_RELATIONSHIP:
        return "Relationship";
    case BOLT_V1_UNBOUND_RELATIONSHIP:
        return "UnboundRelationship";
    case BOLT_V1_PATH:
        return "Path";
    default:
        return "?";
    }
}

const char* BoltProtocolV1_message_name(int16_t code)
{
    switch (code) {
    case BOLT_V1_INIT:
        return "INIT";
    case BOLT_V1_ACK_FAILURE:
        return "ACK_FAILURE";
    case BOLT_V1_RESET:
        return "RESET";
    case BOLT_V1_RUN:
        return "RUN";
    case BOLT_V1_DISCARD_ALL:
        return "DISCARD_ALL";
    case BOLT_V1_PULL_ALL:
        return "PULL_ALL";
    case BOLT_V1_SUCCESS:
        return "SUCCESS";
    case BOLT_V1_RECORD:
        return "RECORD";
    case BOLT_V1_IGNORED:
        return "IGNORED";
    case BOLT_V1_FAILURE:
        return "FAILURE";
    default:
        return "?";
    }
}

struct BoltProtocolV1State* BoltProtocolV1_state(struct BoltConnection* connection)
{
    return (struct BoltProtocolV1State*) (connection->protocol->proto_state);
}

void ensure_failure_data(struct BoltProtocolV1State* state)
{
    if (state->failure_data==NULL) {
        state->failure_data = BoltValue_create();
        BoltValue_format_as_Dictionary(state->failure_data, 2);
        BoltDictionary_set_key(state->failure_data, 0, "code", (int32_t) strlen("code"));
        BoltDictionary_set_key(state->failure_data, 1, "message", (int32_t) strlen("message"));
    }
}

void clear_failure_data(struct BoltProtocolV1State* state)
{
    if (state->failure_data!=NULL) {
        BoltValue_destroy(state->failure_data);
        state->failure_data = NULL;
    }
}

int BoltProtocolV1_load_message(struct BoltConnection* connection, struct BoltMessage* message, int quiet)
{
    if (!quiet) {
        struct BoltProtocolV1State* state = BoltProtocolV1_state(connection);
        BoltLog_message(connection->log, BoltConnection_id(connection), "C", state->next_request_id, message->code,
                message->fields, connection->protocol->structure_name, connection->protocol->message_name);
    }

    struct BoltProtocolV1State* state = BoltProtocolV1_state(connection);
    int prev_cursor = state->tx_buffer->cursor;
    int prev_extent = state->tx_buffer->extent;
    int status = write_message(message, connection->protocol->check_writable_struct, state->tx_buffer, connection->log);
    if (status==BOLT_SUCCESS) {
        push_to_transmission(state->tx_buffer, connection->tx_buffer);
        state->next_request_id += 1;
    }
    else {
        // Reset buffer to its previous state
        state->tx_buffer->cursor = prev_cursor;
        state->tx_buffer->extent = prev_extent;
    }
    return status;
}

int BoltProtocolV1_init(struct BoltConnection* connection, const char* user_agent, const struct BoltValue* auth_token)
{
    struct BoltMessage* init = BoltMessage_create(BOLT_V1_INIT, 2);
    struct BoltProtocolV1State* state = BoltProtocolV1_state(connection);
    TRY(BoltProtocolV1_compile_INIT(init, user_agent, auth_token, 1));
    BoltLog_message(connection->log, BoltConnection_id(connection), "C", state->next_request_id, init->code,
            init->fields, connection->protocol->structure_name, connection->protocol->message_name);
    TRY(BoltProtocolV1_compile_INIT(init, user_agent, auth_token, 0));
    TRY(BoltProtocolV1_load_message(connection, init, 1));
    BoltRequest init_request = BoltConnection_last_request(connection);
    BoltMessage_destroy(init);
    TRY(BoltConnection_send(connection));
    TRY(BoltConnection_fetch_summary(connection, init_request));
    return state->data_type;
}

int BoltProtocolV1_load_discard_request(struct BoltConnection* connection, int32_t n)
{
    if (n>=0) {
        return BOLT_PROTOCOL_VIOLATION;
    }
    else {
        struct BoltProtocolV1State* state = BoltProtocolV1_state(connection);
        TRY(BoltProtocolV1_load_message(connection, state->discard_request, 0));
        return BOLT_SUCCESS;
    }
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
    clear_failure_data(state);
    return BOLT_SUCCESS;
}

int BoltProtocolV1_clear_load_run_request(struct BoltConnection* connection)
{
    struct BoltProtocolV1State* state = BoltProtocolV1_state(connection);
    BoltValue_format_as_String(BoltMessage_param(state->run_request, 0), "", 0);
    BoltValue_format_as_Dictionary(BoltMessage_param(state->run_request, 1), 0);
    return BOLT_SUCCESS;
}

int BoltProtocolV1_set_run_cypher(struct BoltConnection* connection, const char* cypher, const size_t cypher_size,
        int32_t n_parameter)
{
    if (cypher_size<=INT32_MAX) {
        struct BoltProtocolV1State* state = BoltProtocolV1_state(connection);
        struct BoltValue* statement = BoltMessage_param(state->run_request, 0);
        struct BoltValue* params = BoltMessage_param(state->run_request, 1);
        BoltValue_format_as_String(statement, cypher, (int32_t) (cypher_size));
        BoltValue_format_as_Dictionary(params, n_parameter);
        return BOLT_SUCCESS;
    }

    return BOLT_PROTOCOL_VIOLATION;
}

struct BoltValue* BoltProtocolV1_set_run_cypher_parameter(struct BoltConnection* connection, int32_t index,
        const char* name, size_t name_size)
{
    struct BoltProtocolV1State* state = BoltProtocolV1_state(connection);
    struct BoltValue* params = BoltMessage_param(state->run_request, 1);
    BoltDictionary_set_key(params, index, name, (int32_t) name_size);
    return BoltDictionary_value(params, index);
}

int BoltProtocolV1_load_run_request(struct BoltConnection* connection)
{
    struct BoltProtocolV1State* state = BoltProtocolV1_state(connection);
    TRY(BoltProtocolV1_load_message(connection, state->run_request, 0));
    return BOLT_SUCCESS;
}

int BoltProtocolV1_clear_load_begin_tx_request(struct BoltConnection* connection)
{
    struct BoltProtocolV1State* state = BoltProtocolV1_state(connection);
    struct BoltValue* params = BoltMessage_param(state->begin_request, 1);
    BoltValue_format_as_Dictionary(params, 0);
    return BOLT_SUCCESS;
}

int BoltProtocolV1_set_begin_tx_bookmark(struct BoltConnection* connection, struct BoltValue* bookmark_list)
{
    struct BoltProtocolV1State* state = BoltProtocolV1_state(connection);
    struct BoltValue* params = BoltMessage_param(state->begin_request, 1);

    if (bookmark_list==NULL) {
        BoltValue_format_as_Dictionary(params, 0);
        return BOLT_SUCCESS;
    }

    if (BoltValue_type(bookmark_list)!=BOLT_LIST) {
        return BOLT_PROTOCOL_VIOLATION;
    }

    for (int32_t i = 0; i<bookmark_list->size; i++) {
        struct BoltValue* element = BoltList_value(bookmark_list, i);

        if (BoltValue_type(element)!=BOLT_STRING) {
            return BOLT_PROTOCOL_VIOLATION;
        }

        if (element->size>INT32_MAX) {
            return BOLT_PROTOCOL_VIOLATION;
        }
    }

    struct BoltValue* bookmarks;
    if (params->size==0) {
        BoltValue_format_as_Dictionary(params, 1);
        if (BoltDictionary_set_key(params, 0, BOOKMARKS_KEY, BOOKMARKS_KEY_SIZE)) {
            return BOLT_PROTOCOL_VIOLATION;
        }
        bookmarks = BoltDictionary_value(params, 0);
    }
    else {
        bookmarks = BoltDictionary_value(params, 0);
    }

    BoltValue_copy(bookmarks, bookmark_list);

    return BOLT_SUCCESS;
}

int BoltProtocolV1_load_begin_request(struct BoltConnection* connection)
{
    struct BoltProtocolV1State* state = BoltProtocolV1_state(connection);
    TRY(BoltProtocolV1_load_message(connection, state->begin_request, 0));
    TRY(BoltProtocolV1_load_message(connection, state->discard_request, 0));
    return BOLT_SUCCESS;
}

int BoltProtocolV1_load_commit_request(struct BoltConnection* connection)
{
    struct BoltProtocolV1State* state = BoltProtocolV1_state(connection);
    TRY(BoltProtocolV1_load_message(connection, state->commit_request, 0));
    TRY(BoltProtocolV1_load_message(connection, state->discard_request, 0));
    return BOLT_SUCCESS;
}

int BoltProtocolV1_load_rollback_request(struct BoltConnection* connection)
{
    struct BoltProtocolV1State* state = BoltProtocolV1_state(connection);
    TRY(BoltProtocolV1_load_message(connection, state->rollback_request, 0));
    TRY(BoltProtocolV1_load_message(connection, state->discard_request, 0));
    return BOLT_SUCCESS;
}

struct BoltValue* BoltProtocolV1_result_field_names(struct BoltConnection* connection)
{
    struct BoltProtocolV1State* state = BoltProtocolV1_state(connection);
    switch (BoltValue_type(state->result_field_names)) {
    case BOLT_LIST:
        return state->result_field_names;
    default:
        return NULL;
    }
}

struct BoltValue* BoltProtocolV1_result_field_values(struct BoltConnection* connection)
{
    struct BoltProtocolV1State* state = BoltProtocolV1_state(connection);
    switch (state->data_type) {
    case BOLT_V1_RECORD:
        switch (BoltValue_type(state->data)) {
        case BOLT_LIST: {
            struct BoltValue* values = BoltList_value(state->data, 0);
            return values;
        }
        default:
            return NULL;
        }
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

struct BoltValue* BoltProtocolV1_failure(struct BoltConnection* connection)
{
    struct BoltProtocolV1State* state = BoltProtocolV1_state(connection);
    return state->failure_data;
}

char* BoltProtocolV1_last_bookmark(struct BoltConnection* connection)
{
    struct BoltProtocolV1State* state = BoltProtocolV1_state(connection);
    return state->last_bookmark;
}

char* BoltProtocolV1_server(struct BoltConnection* connection)
{
    struct BoltProtocolV1State* state = BoltProtocolV1_state(connection);
    return state->server;
}

BoltRequest BoltProtocolV1_last_request(struct BoltConnection* connection)
{
    struct BoltProtocolV1State* state = BoltProtocolV1_state(connection);
    return state->next_request_id-1;
}

int BoltProtocolV1_is_success_summary(struct BoltConnection* connection)
{
    struct BoltProtocolV1State* state = BoltProtocolV1_state(connection);
    return state->data_type==BOLT_V1_SUCCESS;
}

int BoltProtocolV1_is_failure_summary(struct BoltConnection* connection)
{
    struct BoltProtocolV1State* state = BoltProtocolV1_state(connection);
    return state->data_type==BOLT_V1_FAILURE;
}

int BoltProtocolV1_is_ignored_summary(struct BoltConnection* connection)
{
    struct BoltProtocolV1State* state = BoltProtocolV1_state(connection);
    return state->data_type==BOLT_V1_IGNORED;
}

int16_t BoltProtocolV1_last_data_type(struct BoltConnection* connection)
{
    struct BoltProtocolV1State* state = BoltProtocolV1_state(connection);
    return state->data_type;
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
    state->record_counter = 0;

    state->run_request = create_run_message("", 0, 0);
    state->begin_request = create_run_message("BEGIN", 5, 0);
    state->commit_request = create_run_message("COMMIT", 6, 0);
    state->rollback_request = create_run_message("ROLLBACK", 8, 0);

    state->discard_request = BoltMessage_create(BOLT_V1_DISCARD_ALL, 0);

    state->pull_request = BoltMessage_create(BOLT_V1_PULL_ALL, 0);

    state->reset_request = BoltMessage_create(BOLT_V1_RESET, 0);

    state->data_type = BOLT_V1_RECORD;
    state->data = BoltValue_create();
    return state;
}

void BoltProtocolV1_destroy_state(struct BoltProtocolV1State* state)
{
    if (state==NULL) return;

    BoltBuffer_destroy(state->tx_buffer);
    BoltBuffer_destroy(state->rx_buffer);

    BoltMessage_destroy(state->run_request);
    BoltMessage_destroy(state->begin_request);
    BoltMessage_destroy(state->commit_request);
    BoltMessage_destroy(state->rollback_request);

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

    int32_t
            size = marker & 0x0F;
    BoltValue_format_as_List(state->data, size);
    for (int i = 0; i<size; i++) {
        TRY(unload(connection->protocol->check_readable_struct, state->rx_buffer, BoltList_value(state->data, i),
                connection->log));
    }
    if (code==BOLT_V1_RECORD) {
        if (state->record_counter<MAX_LOGGED_RECORDS) {
            BoltLog_message(connection->log, BoltConnection_id(connection), "S", state->response_counter, code,
                    state->data, connection->protocol->structure_name, connection->protocol->message_name);
        }
        state->record_counter += 1;
    }
    else {
        if (state->record_counter>MAX_LOGGED_RECORDS) {
            BoltLog_info(connection->log, "[%s]: S[%d]: Received %llu more records", BoltConnection_id(connection),
                    state->response_counter,
                    state->record_counter-MAX_LOGGED_RECORDS);
        }
        state->record_counter = 0;
        BoltLog_message(connection->log, BoltConnection_id(connection), "S", state->response_counter, code, state->data,
                connection->protocol->structure_name, connection->protocol->message_name);
    }
    return BOLT_SUCCESS;
}

void BoltProtocolV1_extract_metadata(struct BoltConnection* connection, struct BoltValue* metadata)
{
    struct BoltProtocolV1State* state = BoltProtocolV1_state(connection);

    switch (BoltValue_type(metadata)) {
    case BOLT_DICTIONARY: {
        for (int32_t i = 0; i<metadata->size; i++) {
            struct BoltValue* key = BoltDictionary_key(metadata, i);

            if (BoltString_equals(key, BOOKMARK_KEY, BOOKMARK_KEY_SIZE)) {
                struct BoltValue* value = BoltDictionary_value(metadata, i);
                switch (BoltValue_type(value)) {
                case BOLT_STRING: {
                    memset(state->last_bookmark, 0, MAX_BOOKMARK_SIZE);
                    memcpy(state->last_bookmark, BoltString_get(value), (size_t) (value->size));
                    BoltLog_info(connection->log, "[%s]: <SET last_bookmark=\"%s\">", BoltConnection_id(connection),
                            state->last_bookmark);
                    break;
                }
                default:
                    break;
                }
            }
            else if (BoltString_equals(key, FIELDS_KEY, FIELDS_KEY_SIZE)) {
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
                    BoltLog_value(connection->log, "[%s]: <SET result_field_names=%s>", BoltConnection_id(connection),
                            target_value, connection->protocol->structure_name);
                    break;
                }
                default:
                    break;
                }
            }
            else if (BoltString_equals(key, SERVER_KEY, SERVER_KEY_SIZE)) {
                struct BoltValue* value = BoltDictionary_value(metadata, i);
                switch (BoltValue_type(value)) {
                case BOLT_STRING: {
                    memset(state->server, 0, MAX_SERVER_SIZE);
                    memcpy(state->server, BoltString_get(value), (size_t) (value->size));
                    BoltLog_info(connection->log, "[%s]: <SET server=\"%s\">", BoltConnection_id(connection),
                            state->server);
                    break;
                }
                default:
                    break;
                }
            }
            else if (BoltString_equals(key, FAILURE_CODE_KEY, FAILURE_CODE_KEY_SIZE)
                    && state->data_type==BOLT_V1_FAILURE) {
                struct BoltValue* value = BoltDictionary_value(metadata, i);
                switch (BoltValue_type(value)) {
                case BOLT_STRING: {
                    ensure_failure_data(state);

                    struct BoltValue* target_value = BoltDictionary_value(state->failure_data, 0);
                    BoltValue_format_as_String(target_value, BoltString_get(value), value->size);

                    BoltLog_value(connection->log, "[%s]: <FAILURE code=\"%s\">", BoltConnection_id(connection),
                            target_value, connection->protocol->structure_name);

                    break;
                }
                default:
                    break;
                }
            }
            else if (BoltString_equals(key, FAILURE_MESSAGE_KEY, FAILURE_MESSAGE_KEY_SIZE)
                    && state->data_type==BOLT_V1_FAILURE) {
                struct BoltValue* value = BoltDictionary_value(metadata, i);
                switch (BoltValue_type(value)) {
                case BOLT_STRING: {
                    ensure_failure_data(state);

                    struct BoltValue* target_value = BoltDictionary_value(state->failure_data, 1);
                    BoltValue_format_as_String(target_value, BoltString_get(value), value->size);

                    BoltLog_value(connection->log, "[%s]: <FAILURE message=\"%s\">", BoltConnection_id(connection),
                            target_value, connection->protocol->structure_name);

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
                int32_t
                        index = state->result_metadata->size;
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

int BoltProtocolV1_fetch(struct BoltConnection* connection, BoltRequest request_id)
{
    struct BoltProtocolV1State* state = BoltProtocolV1_state(connection);
    BoltRequest response_id;
    do {
        char header[2];
        int status = BoltConnection_receive(connection, &header[0], 2);
        if (status!=BOLT_SUCCESS) {
            return -1;
        }
        uint16_t chunk_size = char_to_uint16be(header);
        BoltBuffer_compact(state->rx_buffer);
        while (chunk_size!=0) {
            status = BoltConnection_receive(connection, BoltBuffer_load_pointer(state->rx_buffer, chunk_size),
                    chunk_size);
            if (status!=BOLT_SUCCESS) {
                return -1;
            }
            status = BoltConnection_receive(connection, &header[0], 2);
            if (status!=BOLT_SUCCESS) {
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

int BoltProtocolV1_set_tx_timeout_unsupported(struct BoltConnection* connection, int64_t n)
{
    UNUSED(connection);
    UNUSED(n);
    return BOLT_PROTOCOL_UNSUPPORTED;
}

int BoltProtocolV1_set_tx_bookmark_ignore(struct BoltConnection* connection, struct BoltValue* value)
{
    UNUSED(connection);
    UNUSED(value);
    // we will ignore bookmarks with this version of the protocol
    return BOLT_SUCCESS;
}

int BoltProtocolV1_set_tx_metadata_unsupported(struct BoltConnection* connection, struct BoltValue* value)
{
    UNUSED(connection);
    UNUSED(value);
    return BOLT_PROTOCOL_UNSUPPORTED;
}

int BoltProtocolV1_goodbye_noop(struct BoltConnection* connection)
{
    UNUSED(connection);
    return BOLT_SUCCESS;
}

struct BoltProtocol* BoltProtocolV1_create_protocol()
{
    struct BoltProtocol* protocol = BoltMem_allocate(sizeof(struct BoltProtocol));

    protocol->proto_state = BoltProtocolV1_create_state();

    protocol->message_name = &BoltProtocolV1_message_name;
    protocol->structure_name = &BoltProtocolV1_structure_name;

    protocol->check_readable_struct = &BoltProtocolV1_check_readable_struct_signature;
    protocol->check_writable_struct = &BoltProtocolV1_check_writable_struct_signature;

    protocol->init = &BoltProtocolV1_init;
    protocol->goodbye = &BoltProtocolV1_goodbye_noop;

    protocol->clear_run = &BoltProtocolV1_clear_load_run_request;
    protocol->set_run_cypher = &BoltProtocolV1_set_run_cypher;
    protocol->set_run_cypher_parameter = &BoltProtocolV1_set_run_cypher_parameter;
    protocol->set_run_bookmark = &BoltProtocolV1_set_tx_bookmark_ignore;
    protocol->set_run_tx_timeout = &BoltProtocolV1_set_tx_timeout_unsupported;
    protocol->set_run_tx_metadata = &BoltProtocolV1_set_tx_metadata_unsupported;
    protocol->load_run = &BoltProtocolV1_load_run_request;

    protocol->clear_begin_tx = &BoltProtocolV1_clear_load_begin_tx_request;
    protocol->set_begin_tx_bookmark = &BoltProtocolV1_set_begin_tx_bookmark;
    protocol->set_begin_tx_timeout = &BoltProtocolV1_set_tx_timeout_unsupported;
    protocol->set_begin_tx_metadata = &BoltProtocolV1_set_tx_metadata_unsupported;
    protocol->load_begin_tx = &BoltProtocolV1_load_begin_request;

    protocol->load_commit_tx = &BoltProtocolV1_load_commit_request;
    protocol->load_rollback_tx = &BoltProtocolV1_load_rollback_request;
    protocol->load_discard = &BoltProtocolV1_load_discard_request;
    protocol->load_pull = &BoltProtocolV1_load_pull_request;
    protocol->load_reset = &BoltProtocolV1_load_reset_request;

    protocol->last_request = &BoltProtocolV1_last_request;

    protocol->field_names = &BoltProtocolV1_result_field_names;
    protocol->field_values = &BoltProtocolV1_result_field_values;
    protocol->metadata = &BoltProtocolV1_result_metadata;
    protocol->failure = &BoltProtocolV1_failure;

    protocol->last_data_type = &BoltProtocolV1_last_data_type;
    protocol->last_bookmark = &BoltProtocolV1_last_bookmark;
    protocol->server = &BoltProtocolV1_server;
    protocol->id = NULL;

    protocol->is_failure_summary = &BoltProtocolV1_is_failure_summary;
    protocol->is_success_summary = &BoltProtocolV1_is_success_summary;
    protocol->is_ignored_summary = &BoltProtocolV1_is_ignored_summary;

    protocol->fetch = &BoltProtocolV1_fetch;

    return protocol;
}

void BoltProtocolV1_destroy_protocol(struct BoltProtocol* protocol)
{
    if (protocol!=NULL) {
        BoltProtocolV1_destroy_state(protocol->proto_state);
    }

    BoltMem_deallocate(protocol, sizeof(struct BoltProtocol));
}
