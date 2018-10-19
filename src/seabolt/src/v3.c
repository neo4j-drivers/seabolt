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

#include <string.h>

#include "config-impl.h"
#include "buffering.h"
#include "logging.h"
#include "mem.h"
#include "protocol.h"
#include "v3.h"

#define MASK "********"
#define MASK_SIZE 8
#define CREDENTIALS_KEY "credentials"
#define CREDENTIALS_KEY_SIZE 11
#define USER_AGENT_KEY "user_agent"
#define USER_AGENT_KEY_SIZE 10
#define BOOKMARKS_KEY "bookmarks"
#define BOOKMARKS_KEY_SIZE 9
#define TX_TIMEOUT_KEY "tx_timeout"
#define TX_TIMEOUT_KEY_SIZE 10
#define TX_METADATA_KEY "tx_metadata"
#define TX_METADATA_KEY_SIZE 11
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
#define CONNECTION_ID_KEY "connection_id"
#define CONNECTION_ID_KEY_SIZE 13

#define INITIAL_TX_BUFFER_SIZE 8192
#define INITIAL_RX_BUFFER_SIZE 8192
#define MAX_BOOKMARK_SIZE 40
#define MAX_SERVER_SIZE 200
#define MAX_CONNECTION_ID_SIZE 200
#define MAX_LOGGED_RECORDS 3

#define char_to_uint16be(array) ((uint8_t)(header[0]) << 8) | (uint8_t)(header[1]);

#define TRY(code) { int status_try = (code); if (status_try != BOLT_SUCCESS) { return status_try; } }

struct BoltProtocolV3State {
    // These buffers exclude chunk headers.
    struct BoltBuffer* tx_buffer;
    struct BoltBuffer* rx_buffer;

    /// The product name and version of the remote server
    char* server;
    /// A BoltValue containing field names for the active result
    struct BoltValue* result_field_names;
    /// A BoltValue containing metadata fields
    struct BoltValue* result_metadata;
    /// A BoltValue containing error code and message
    struct BoltValue* failure_data;
    /// The last bookmark received from the server
    char* last_bookmark;
    /// A connection identifier assigned by the server
    char* connection_id;

    bolt_request next_request_id;
    bolt_request response_counter;
    unsigned long long record_counter;

    struct BoltMessage* run_request;
    struct BoltMessage* begin_request;
    struct BoltMessage* commit_request;
    struct BoltMessage* rollback_request;

    struct BoltMessage* discard_request;
    struct BoltMessage* pull_request;
    struct BoltMessage* reset_request;

    /// Holder for fetched data and metadata
    int16_t data_type;
    struct BoltValue* data;
};

int _clear_begin_tx(struct BoltMessage* request)
{
    struct BoltValue* metadata = BoltMessage_param(request, 0);
    BoltValue_format_as_Dictionary(metadata, 0);
    return BOLT_SUCCESS;
}

int _clear_run(struct BoltMessage* request)
{
    struct BoltValue* statement = BoltMessage_param(request, 0);
    struct BoltValue* params = BoltMessage_param(request, 1);
    struct BoltValue* metadata = BoltMessage_param(request, 2);

    BoltValue_format_as_String(statement, "", 0);
    BoltValue_format_as_Dictionary(params, 0);
    BoltValue_format_as_Dictionary(metadata, 0);

    return BOLT_SUCCESS;
}

void _ensure_failure_data(struct BoltProtocolV3State* state)
{
    if (state->failure_data==NULL) {
        state->failure_data = BoltValue_create();
        BoltValue_format_as_Dictionary(state->failure_data, 2);
        BoltDictionary_set_key(state->failure_data, 0, FAILURE_CODE_KEY, FAILURE_CODE_KEY_SIZE);
        BoltDictionary_set_key(state->failure_data, 1, FAILURE_MESSAGE_KEY, FAILURE_MESSAGE_KEY_SIZE);
    }
}

void _clear_failure_data(struct BoltProtocolV3State* state)
{
    if (state->failure_data!=NULL) {
        BoltValue_destroy(state->failure_data);
        state->failure_data = NULL;
    }
}

int BoltProtocolV3_check_readable_struct_signature(int16_t signature)
{
    switch (signature) {
    case BOLT_V3_SUCCESS:
    case BOLT_V3_FAILURE:
    case BOLT_V3_IGNORED:
    case BOLT_V3_RECORD:
    case BOLT_V3_NODE:
    case BOLT_V3_RELATIONSHIP:
    case BOLT_V3_UNBOUND_RELATIONSHIP:
    case BOLT_V3_PATH:
    case BOLT_V3_POINT_2D:
    case BOLT_V3_POINT_3D:
    case BOLT_V3_LOCAL_DATE:
    case BOLT_V3_LOCAL_DATE_TIME:
    case BOLT_V3_LOCAL_TIME:
    case BOLT_V3_OFFSET_TIME:
    case BOLT_V3_OFFSET_DATE_TIME:
    case BOLT_V3_ZONED_DATE_TIME:
    case BOLT_V3_DURATION:
        return 1;
    }

    return 0;
}

int BoltProtocolV3_check_writable_struct_signature(int16_t signature)
{
    switch (signature) {
    case BOLT_V3_RESET:
    case BOLT_V3_DISCARD_ALL:
    case BOLT_V3_PULL_ALL:
    case BOLT_V3_POINT_2D:
    case BOLT_V3_POINT_3D:
    case BOLT_V3_LOCAL_DATE:
    case BOLT_V3_LOCAL_DATE_TIME:
    case BOLT_V3_LOCAL_TIME:
    case BOLT_V3_OFFSET_TIME:
    case BOLT_V3_OFFSET_DATE_TIME:
    case BOLT_V3_ZONED_DATE_TIME:
    case BOLT_V3_DURATION:
    case BOLT_V3_HELLO:
    case BOLT_V3_RUN:
    case BOLT_V3_BEGIN:
    case BOLT_V3_COMMIT:
    case BOLT_V3_ROLLBACK:
    case BOLT_V3_GOODBYE:
        return 1;
    }

    return 0;
}

const char* BoltProtocolV3_structure_name(int16_t code)
{
    switch (code) {
    case BOLT_V3_NODE:
        return "Node";
    case BOLT_V3_RELATIONSHIP:
        return "Relationship";
    case BOLT_V3_UNBOUND_RELATIONSHIP:
        return "UnboundRelationship";
    case BOLT_V3_PATH:
        return "Path";
    case BOLT_V3_POINT_2D:
        return "Point2D";
    case BOLT_V3_POINT_3D:
        return "Point3D";
    case BOLT_V3_LOCAL_DATE:
        return "LocalDate";
    case BOLT_V3_LOCAL_TIME:
        return "LocalTime";
    case BOLT_V3_LOCAL_DATE_TIME:
        return "LocalDateTime";
    case BOLT_V3_OFFSET_TIME:
        return "OffsetTime";
    case BOLT_V3_OFFSET_DATE_TIME:
        return "OffsetDateTime";
    case BOLT_V3_ZONED_DATE_TIME:
        return "ZonedDateTime";
    case BOLT_V3_DURATION:
        return "Duration";
    default:
        return "?";
    }
}

const char* BoltProtocolV3_message_name(int16_t code)
{
    switch (code) {
    case BOLT_V3_RESET:
        return "RESET";
    case BOLT_V3_DISCARD_ALL:
        return "DISCARD_ALL";
    case BOLT_V3_PULL_ALL:
        return "PULL_ALL";
    case BOLT_V3_SUCCESS:
        return "SUCCESS";
    case BOLT_V3_RECORD:
        return "RECORD";
    case BOLT_V3_IGNORED:
        return "IGNORED";
    case BOLT_V3_FAILURE:
        return "FAILURE";
    case BOLT_V3_HELLO:
        return "HELLO";
    case BOLT_V3_RUN:
        return "RUN";
    case BOLT_V3_BEGIN:
        return "BEGIN";
    case BOLT_V3_COMMIT:
        return "COMMIT";
    case BOLT_V3_ROLLBACK:
        return "ROLLBACK";
    case BOLT_V3_GOODBYE:
        return "GOODBYE";
    default:
        return "?";
    }
}

struct BoltProtocolV3State* BoltProtocolV3_state(struct BoltConnection* connection)
{
    return (struct BoltProtocolV3State*) (connection->protocol->proto_state);
}

struct BoltProtocolV3State* BoltProtocolV3_create_state()
{
    struct BoltProtocolV3State* state = BoltMem_allocate(sizeof(struct BoltProtocolV3State));

    state->tx_buffer = BoltBuffer_create(INITIAL_TX_BUFFER_SIZE);
    state->rx_buffer = BoltBuffer_create(INITIAL_RX_BUFFER_SIZE);

    state->server = BoltMem_allocate(MAX_SERVER_SIZE);
    memset(state->server, 0, MAX_SERVER_SIZE);
    state->connection_id = BoltMem_allocate(MAX_CONNECTION_ID_SIZE);
    memset(state->connection_id, 0, MAX_CONNECTION_ID_SIZE);
    state->result_field_names = BoltValue_create();
    state->result_metadata = BoltValue_create();
    BoltValue_format_as_Dictionary(state->result_metadata, 0);
    state->failure_data = NULL;
    state->last_bookmark = BoltMem_allocate(MAX_BOOKMARK_SIZE);
    memset(state->last_bookmark, 0, MAX_BOOKMARK_SIZE);

    state->next_request_id = 0;
    state->response_counter = 0;

    state->begin_request = BoltMessage_create(BOLT_V3_BEGIN, 1);
    _clear_begin_tx(state->begin_request);

    state->run_request = BoltMessage_create(BOLT_V3_RUN, 3);
    _clear_run(state->run_request);

    state->commit_request = BoltMessage_create(BOLT_V3_COMMIT, 0);
    state->rollback_request = BoltMessage_create(BOLT_V3_ROLLBACK, 0);

    state->discard_request = BoltMessage_create(BOLT_V3_DISCARD_ALL, 0);

    state->pull_request = BoltMessage_create(BOLT_V3_PULL_ALL, 0);

    state->reset_request = BoltMessage_create(BOLT_V3_RESET, 0);

    state->data_type = BOLT_V3_RECORD;
    state->data = BoltValue_create();
    return state;
}

void BoltProtocolV3_destroy_state(struct BoltProtocolV3State* state)
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

    BoltMem_deallocate(state->connection_id, MAX_CONNECTION_ID_SIZE);
    BoltMem_deallocate(state->server, MAX_SERVER_SIZE);
    if (state->failure_data!=NULL) {
        BoltValue_destroy(state->failure_data);
    }
    BoltValue_destroy(state->result_field_names);
    BoltValue_destroy(state->result_metadata);
    BoltMem_deallocate(state->last_bookmark, MAX_BOOKMARK_SIZE);

    BoltValue_destroy(state->data);

    BoltMem_deallocate(state, sizeof(struct BoltProtocolV3State));
}

int BoltProtocolV3_load_message(struct BoltConnection* connection, struct BoltMessage* message, int quiet)
{
    struct BoltProtocolV3State* state = BoltProtocolV3_state(connection);

    if (!quiet) {
        BoltLog_message(connection->log, "C", state->next_request_id, message->code, message->fields,
                connection->protocol->structure_name, connection->protocol->message_name);
    }

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

int
BoltProtocolV3_compile_HELLO(struct BoltMessage* message, const char* user_agent, const struct BoltValue* auth_token,
        int mask_secure_fields)
{
    struct BoltValue* params = BoltMessage_param(message, 0);
    BoltValue_format_as_Dictionary(params, auth_token->size+1);

    // copy auth_token
    for (int32_t i = 0; i<auth_token->size; i++) {
        struct BoltValue* src_key = BoltDictionary_key(auth_token, i);
        struct BoltValue* src_value = BoltDictionary_value(auth_token, i);

        struct BoltValue* dest_key = BoltDictionary_key(params, i);
        struct BoltValue* dest_value = BoltDictionary_value(params, i);

        BoltValue_copy(dest_key, src_key);
        BoltValue_copy(dest_value, src_value);
    }

    // add user_agent
    BoltDictionary_set_key(params, params->size-1, USER_AGENT_KEY, USER_AGENT_KEY_SIZE);
    struct BoltValue* user_agent_value = BoltDictionary_value(params, params->size-1);
    BoltValue_format_as_String(user_agent_value, user_agent, (int32_t) (strlen(user_agent)));

    if (mask_secure_fields) {
        struct BoltValue* secure_value = BoltDictionary_value_by_key(params, CREDENTIALS_KEY, CREDENTIALS_KEY_SIZE);
        if (secure_value!=NULL) {
            BoltValue_format_as_String(secure_value, MASK, MASK_SIZE);
        }
    }

    return BOLT_SUCCESS;
}

int BoltProtocolV3_hello(struct BoltConnection* connection, const char* user_agent, const struct BoltValue* auth_token)
{
    struct BoltMessage* hello = BoltMessage_create(BOLT_V3_HELLO, 1);
    struct BoltProtocolV3State* state = BoltProtocolV3_state(connection);
    TRY(BoltProtocolV3_compile_HELLO(hello, user_agent, auth_token, 1));
    BoltLog_message(connection->log, "C", state->next_request_id, hello->code, hello->fields,
            connection->protocol->structure_name, connection->protocol->message_name);
    TRY(BoltProtocolV3_compile_HELLO(hello, user_agent, auth_token, 0));
    TRY(BoltProtocolV3_load_message(connection, hello, 1));
    bolt_request hello_request = BoltConnection_last_request(connection);
    BoltMessage_destroy(hello);
    TRY(BoltConnection_send(connection));
    TRY(BoltConnection_fetch_summary(connection, hello_request));
    return state->data_type;
}

int BoltProtocolV3_goodbye(struct BoltConnection* connection)
{
    struct BoltMessage* goodbye = BoltMessage_create(BOLT_V3_GOODBYE, 0);
    TRY(BoltProtocolV3_load_message(connection, goodbye, 0));
    BoltMessage_destroy(goodbye);
    int status = BoltConnection_send(connection);
    if (status!=BOLT_SUCCESS) {
        BoltLog_warning(connection->log, "unable to complete GOODBYE call, returned code is %x", status);
    }
    return BOLT_SUCCESS;
}

int BoltProtocolV3_clear_begin_tx(struct BoltConnection* connection)
{
    struct BoltProtocolV3State* state = BoltProtocolV3_state(connection);
    return _clear_begin_tx(state->begin_request);
}

int _set_tx_bookmark(struct BoltValue* metadata, struct BoltValue* bookmark_list, const struct BoltLog* log,
        name_resolver_func struct_resolver)
{
    BoltLog_value(log, "setting transaction_bookmark: %s", bookmark_list, struct_resolver);
    struct BoltValue* bookmarks_value = BoltDictionary_value_by_key(metadata, BOOKMARKS_KEY, BOOKMARKS_KEY_SIZE);

    if (bookmark_list==NULL) {
        BoltLog_debug(log, "passed bookmarks list is NULL");
        if (bookmarks_value!=NULL) {
            BoltLog_debug(log, "clearing out already set bookmarks");
            BoltValue_format_as_List(bookmarks_value, 0);
        }

        return BOLT_SUCCESS;
    }

    if (BoltValue_type(bookmark_list)!=BOLT_LIST) {
        BoltLog_debug(log, "passed bookmarks list is not of type BOLT_LIST, it is: %d", BoltValue_type(bookmark_list));
        return BOLT_PROTOCOL_VIOLATION;
    }

    for (int32_t i = 0; i<bookmark_list->size; i++) {
        struct BoltValue* element = BoltList_value(bookmark_list, i);

        if (BoltValue_type(element)!=BOLT_STRING) {
            BoltLog_debug(log, "passed bookmark at position %d is not of type BOLT_STRING, it is: %d", i,
                    BoltValue_type(element));
            return BOLT_PROTOCOL_VIOLATION;
        }

        if (element->size>INT32_MAX) {
            BoltLog_debug(log, "passed bookmark at position %d exceeds maximum size %d", i, element->size);
            return BOLT_PROTOCOL_VIOLATION;
        }
    }

    if (bookmarks_value==NULL) {
        BoltLog_debug(log, "metadata map doesn't contain a key for bookmarks, adding an entry.");
        int32_t index = metadata->size;
        BoltValue_format_as_Dictionary(metadata, metadata->size+1);
        BoltDictionary_set_key(metadata, index, BOOKMARKS_KEY, BOOKMARKS_KEY_SIZE);
        bookmarks_value = BoltDictionary_value(metadata, index);
    }

    BoltLog_debug(log, "copying passed in bookmarks list into metadata map");
    BoltValue_copy(bookmarks_value, bookmark_list);

    return BOLT_SUCCESS;
}

int _set_tx_timeout(struct BoltValue* metadata, int64_t tx_timeout)
{
    struct BoltValue* tx_timeout_value = BoltDictionary_value_by_key(metadata, TX_TIMEOUT_KEY, TX_TIMEOUT_KEY_SIZE);

    if (tx_timeout<0) {
        if (tx_timeout_value!=NULL) {
            BoltValue_format_as_Null(tx_timeout_value);
        }

        return BOLT_SUCCESS;
    }

    if (tx_timeout_value==NULL) {
        int32_t index = metadata->size;
        BoltValue_format_as_Dictionary(metadata, metadata->size+1);
        BoltDictionary_set_key(metadata, index, TX_TIMEOUT_KEY, TX_TIMEOUT_KEY_SIZE);
        tx_timeout_value = BoltDictionary_value(metadata, index);
    }

    BoltValue_format_as_Integer(tx_timeout_value, tx_timeout);

    return BOLT_SUCCESS;
}

int _set_tx_metadata(struct BoltValue* metadata, struct BoltValue* tx_metadata)
{
    struct BoltValue* tx_metadata_value = BoltDictionary_value_by_key(metadata, TX_METADATA_KEY,
            TX_METADATA_KEY_SIZE);

    if (tx_metadata==NULL) {
        if (tx_metadata_value!=NULL) {
            BoltValue_format_as_Dictionary(tx_metadata_value, 0);
        }

        return BOLT_SUCCESS;
    }

    if (BoltValue_type(tx_metadata)!=BOLT_DICTIONARY) {
        return BOLT_PROTOCOL_VIOLATION;
    }

    for (int32_t i = 0; i<tx_metadata->size; i++) {
        struct BoltValue* key = BoltDictionary_key(tx_metadata, i);

        if (BoltValue_type(key)!=BOLT_STRING) {
            return BOLT_PROTOCOL_VIOLATION;
        }
    }

    if (tx_metadata_value==NULL) {
        int32_t index = metadata->size;
        BoltValue_format_as_Dictionary(metadata, metadata->size+1);
        BoltDictionary_set_key(metadata, index, TX_METADATA_KEY, TX_METADATA_KEY_SIZE);
        tx_metadata_value = BoltDictionary_value(metadata, index);
    }

    BoltValue_copy(tx_metadata_value, tx_metadata);

    return BOLT_SUCCESS;
}

int BoltProtocolV3_set_begin_tx_bookmark(struct BoltConnection* connection, struct BoltValue* bookmark_list)
{
    struct BoltProtocolV3State* state = BoltProtocolV3_state(connection);
    struct BoltValue* metadata = BoltMessage_param(state->begin_request, 0);
    return _set_tx_bookmark(metadata, bookmark_list, connection->log, connection->protocol->structure_name);
}

int BoltProtocolV3_set_begin_tx_timeout(struct BoltConnection* connection, int64_t tx_timeout)
{
    struct BoltProtocolV3State* state = BoltProtocolV3_state(connection);
    struct BoltValue* metadata = BoltMessage_param(state->begin_request, 0);
    return _set_tx_timeout(metadata, tx_timeout);
}

int BoltProtocolV3_set_begin_tx_metadata(struct BoltConnection* connection, struct BoltValue* tx_metadata)
{
    struct BoltProtocolV3State* state = BoltProtocolV3_state(connection);
    struct BoltValue* metadata = BoltMessage_param(state->begin_request, 0);
    return _set_tx_metadata(metadata, tx_metadata);
}

int BoltProtocolV3_load_begin_tx(struct BoltConnection* connection)
{
    struct BoltProtocolV3State* state = BoltProtocolV3_state(connection);
    TRY(BoltProtocolV3_load_message(connection, state->begin_request, 0));
    return BOLT_SUCCESS;
}

int BoltProtocolV3_load_commit_tx(struct BoltConnection* connection)
{
    struct BoltProtocolV3State* state = BoltProtocolV3_state(connection);
    TRY(BoltProtocolV3_load_message(connection, state->commit_request, 0));
    return BOLT_SUCCESS;
}

int BoltProtocolV3_load_rollback_tx(struct BoltConnection* connection)
{
    struct BoltProtocolV3State* state = BoltProtocolV3_state(connection);
    TRY(BoltProtocolV3_load_message(connection, state->rollback_request, 0));
    return BOLT_SUCCESS;
}

int BoltProtocolV3_clear_run(struct BoltConnection* connection)
{
    struct BoltProtocolV3State* state = BoltProtocolV3_state(connection);
    return _clear_run(state->run_request);
}

int BoltProtocolV3_set_run_bookmark(struct BoltConnection* connection, struct BoltValue* bookmark_list)
{
    struct BoltProtocolV3State* state = BoltProtocolV3_state(connection);
    struct BoltValue* metadata = BoltMessage_param(state->run_request, 2);
    return _set_tx_bookmark(metadata, bookmark_list, connection->log, connection->protocol->structure_name);
}

int BoltProtocolV3_set_run_tx_metadata(struct BoltConnection* connection, struct BoltValue* tx_metadata)
{
    struct BoltProtocolV3State* state = BoltProtocolV3_state(connection);
    struct BoltValue* metadata = BoltMessage_param(state->run_request, 2);
    return _set_tx_metadata(metadata, tx_metadata);
}

int BoltProtocolV3_set_run_tx_timeout(struct BoltConnection* connection, int64_t tx_timeout)
{
    struct BoltProtocolV3State* state = BoltProtocolV3_state(connection);
    struct BoltValue* metadata = BoltMessage_param(state->run_request, 2);
    return _set_tx_timeout(metadata, tx_timeout);
}

int BoltProtocolV3_set_run_cypher(struct BoltConnection* connection, const char* cypher, const size_t cypher_size,
        int32_t n_parameter)
{
    if (cypher_size<=INT32_MAX) {
        struct BoltProtocolV3State* state = BoltProtocolV3_state(connection);
        struct BoltValue* statement = BoltMessage_param(state->run_request, 0);
        struct BoltValue* params = BoltMessage_param(state->run_request, 1);
        BoltValue_format_as_String(statement, cypher, (int32_t) (cypher_size));
        BoltValue_format_as_Dictionary(params, n_parameter);
        return BOLT_SUCCESS;
    }

    return BOLT_PROTOCOL_VIOLATION;
}

struct BoltValue*
BoltProtocolV3_set_run_cypher_parameter(struct BoltConnection* connection, int32_t index, const char* name,
        size_t name_size)
{
    struct BoltProtocolV3State* state = BoltProtocolV3_state(connection);
    struct BoltValue* params = BoltMessage_param(state->run_request, 1);
    BoltDictionary_set_key(params, index, name, name_size);
    return BoltDictionary_value(params, index);
}

int BoltProtocolV3_load_run(struct BoltConnection* connection)
{
    struct BoltProtocolV3State* state = BoltProtocolV3_state(connection);
    TRY(BoltProtocolV3_load_message(connection, state->run_request, 0));
    return BOLT_SUCCESS;
}

int BoltProtocolV3_load_discard(struct BoltConnection* connection, int32_t n)
{
    if (n>=0) {
        return BOLT_PROTOCOL_VIOLATION;
    }
    else {
        struct BoltProtocolV3State* state = BoltProtocolV3_state(connection);
        TRY(BoltProtocolV3_load_message(connection, state->discard_request, 0));
        return BOLT_SUCCESS;
    }
}

int BoltProtocolV3_load_pull(struct BoltConnection* connection, int32_t n)
{
    if (n>=0) {
        return BOLT_PROTOCOL_VIOLATION;
    }
    else {
        struct BoltProtocolV3State* state = BoltProtocolV3_state(connection);
        TRY(BoltProtocolV3_load_message(connection, state->pull_request, 0));
        return BOLT_SUCCESS;
    }
}

int BoltProtocolV3_load_reset(struct BoltConnection* connection)
{
    struct BoltProtocolV3State* state = BoltProtocolV3_state(connection);
    TRY(BoltProtocolV3_load_message(connection, state->reset_request, 0));
    _clear_failure_data(state);
    return BOLT_SUCCESS;
}

struct BoltValue* BoltProtocolV3_result_field_names(struct BoltConnection* connection)
{
    struct BoltProtocolV3State* state = BoltProtocolV3_state(connection);
    switch (BoltValue_type(state->result_field_names)) {
    case BOLT_LIST:
        return state->result_field_names;
    default:
        return NULL;
    }
}

struct BoltValue* BoltProtocolV3_result_field_values(struct BoltConnection* connection)
{
    struct BoltProtocolV3State* state = BoltProtocolV3_state(connection);
    switch (state->data_type) {
    case BOLT_V3_RECORD:
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

struct BoltValue* BoltProtocolV3_result_metadata(struct BoltConnection* connection)
{
    struct BoltProtocolV3State* state = BoltProtocolV3_state(connection);
    switch (BoltValue_type(state->result_metadata)) {
    case BOLT_DICTIONARY:
        return state->result_metadata;
    default:
        return NULL;
    }
}

struct BoltValue* BoltProtocolV3_failure(struct BoltConnection* connection)
{
    struct BoltProtocolV3State* state = BoltProtocolV3_state(connection);
    return state->failure_data;
}

char* BoltProtocolV3_last_bookmark(struct BoltConnection* connection)
{
    struct BoltProtocolV3State* state = BoltProtocolV3_state(connection);
    return state->last_bookmark;
}

char* BoltProtocolV3_server(struct BoltConnection* connection)
{
    struct BoltProtocolV3State* state = BoltProtocolV3_state(connection);
    return state->server;
}

bolt_request BoltProtocolV3_last_request(struct BoltConnection* connection)
{
    struct BoltProtocolV3State* state = BoltProtocolV3_state(connection);
    return state->next_request_id-1;
}

int BoltProtocolV3_is_success_summary(struct BoltConnection* connection)
{
    struct BoltProtocolV3State* state = BoltProtocolV3_state(connection);
    return state->data_type==BOLT_V3_SUCCESS;
}

int BoltProtocolV3_is_failure_summary(struct BoltConnection* connection)
{
    struct BoltProtocolV3State* state = BoltProtocolV3_state(connection);
    return state->data_type==BOLT_V3_FAILURE;
}

int BoltProtocolV3_is_ignored_summary(struct BoltConnection* connection)
{
    struct BoltProtocolV3State* state = BoltProtocolV3_state(connection);
    return state->data_type==BOLT_V3_IGNORED;
}

int16_t BoltProtocolV3_last_data_type(struct BoltConnection* connection)
{
    struct BoltProtocolV3State* state = BoltProtocolV3_state(connection);
    return state->data_type;
}

int BoltProtocolV3_unload(struct BoltConnection* connection)
{
    struct BoltProtocolV3State* state = BoltProtocolV3_state(connection);
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
        TRY(unload(connection->protocol->check_readable_struct, state->rx_buffer, BoltList_value(state->data, i),
                connection->log));
    }
    if (code==BOLT_V3_RECORD) {
        if (state->record_counter<MAX_LOGGED_RECORDS) {
            BoltLog_message(connection->log, "S", state->response_counter, code, state->data,
                    connection->protocol->structure_name, connection->protocol->message_name);
        }
        state->record_counter += 1;
    }
    else {
        if (state->record_counter>MAX_LOGGED_RECORDS) {
            BoltLog_info(connection->log, "S[%d]: Received %llu more records", state->response_counter,
                    state->record_counter-MAX_LOGGED_RECORDS);
        }
        state->record_counter = 0;
        BoltLog_message(connection->log, "S", state->response_counter, code, state->data,
                connection->protocol->structure_name, connection->protocol->message_name);
    }
    return BOLT_SUCCESS;
}

void BoltProtocolV3_extract_metadata(struct BoltConnection* connection, struct BoltValue* metadata)
{
    struct BoltProtocolV3State* state = BoltProtocolV3_state(connection);

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
                    BoltLog_info(connection->log, "<SET last_bookmark=\"%s\">", state->last_bookmark);
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
                    BoltLog_value(connection->log, "<SET result_field_names=%s>", target_value,
                            connection->protocol->structure_name);
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
                    BoltLog_info(connection->log, "<SET server=\"%s\">", state->server);
                    break;
                }
                default:
                    break;
                }
            }
            else if (BoltString_equals(key, CONNECTION_ID_KEY, CONNECTION_ID_KEY_SIZE)) {
                struct BoltValue* value = BoltDictionary_value(metadata, i);
                switch (BoltValue_type(value)) {
                case BOLT_STRING: {
                    memset(state->connection_id, 0, MAX_CONNECTION_ID_SIZE);
                    memcpy(state->connection_id, BoltString_get(value), (size_t) (value->size));
                    BoltLog_info(connection->log, "<SET connection_id=\"%s\">", state->connection_id);
                    break;
                }
                default:
                    break;
                }
            }
            else if (BoltString_equals(key, FAILURE_CODE_KEY, FAILURE_CODE_KEY_SIZE)
                    && state->data_type==BOLT_V3_FAILURE) {
                struct BoltValue* value = BoltDictionary_value(metadata, i);
                switch (BoltValue_type(value)) {
                case BOLT_STRING: {
                    _ensure_failure_data(state);

                    struct BoltValue* target_value = BoltDictionary_value(state->failure_data, 0);
                    BoltValue_format_as_String(target_value, BoltString_get(value), value->size);

                    BoltLog_value(connection->log, "<FAILURE code=\"%s\">", target_value,
                            connection->protocol->structure_name);

                    break;
                }
                default:
                    break;
                }
            }
            else if (BoltString_equals(key, FAILURE_MESSAGE_KEY, FAILURE_MESSAGE_KEY_SIZE)
                    && state->data_type==BOLT_V3_FAILURE) {
                struct BoltValue* value = BoltDictionary_value(metadata, i);
                switch (BoltValue_type(value)) {
                case BOLT_STRING: {
                    _ensure_failure_data(state);

                    struct BoltValue* target_value = BoltDictionary_value(state->failure_data, 1);
                    BoltValue_format_as_String(target_value, BoltString_get(value), value->size);

                    BoltLog_value(connection->log, "<FAILURE message=\"%s\">", target_value,
                            connection->protocol->structure_name);

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

int BoltProtocolV3_fetch(struct BoltConnection* connection, bolt_request request_id)
{
    struct BoltProtocolV3State* state = BoltProtocolV3_state(connection);
    bolt_request response_id;
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
        TRY(BoltProtocolV3_unload(connection));
        if (state->data_type!=BOLT_V3_RECORD) {
            state->response_counter += 1;

            // Clean existing metadata
            BoltValue_format_as_Dictionary(state->result_metadata, 0);

            if (state->data->size>=1) {
                BoltProtocolV3_extract_metadata(connection, BoltList_value(state->data, 0));
            }
        }
    }
    while (response_id!=request_id);

    if (state->data_type!=BOLT_V3_RECORD) {
        return 0;
    }

    return 1;
}

struct BoltProtocol* BoltProtocolV3_create_protocol()
{
    struct BoltProtocol* protocol = BoltMem_allocate(sizeof(struct BoltProtocol));

    protocol->proto_state = BoltProtocolV3_create_state();

    protocol->message_name = &BoltProtocolV3_message_name;
    protocol->structure_name = &BoltProtocolV3_structure_name;

    protocol->check_readable_struct = &BoltProtocolV3_check_readable_struct_signature;
    protocol->check_writable_struct = &BoltProtocolV3_check_writable_struct_signature;

    protocol->init = &BoltProtocolV3_hello;
    protocol->goodbye = &BoltProtocolV3_goodbye;

    protocol->clear_run = &BoltProtocolV3_clear_run;
    protocol->set_run_cypher = &BoltProtocolV3_set_run_cypher;
    protocol->set_run_cypher_parameter = &BoltProtocolV3_set_run_cypher_parameter;
    protocol->set_run_bookmark = &BoltProtocolV3_set_run_bookmark;
    protocol->set_run_tx_timeout = &BoltProtocolV3_set_run_tx_timeout;
    protocol->set_run_tx_metadata = &BoltProtocolV3_set_run_tx_metadata;
    protocol->load_run = &BoltProtocolV3_load_run;

    protocol->clear_begin_tx = &BoltProtocolV3_clear_begin_tx;
    protocol->set_begin_tx_bookmark = &BoltProtocolV3_set_begin_tx_bookmark;
    protocol->set_begin_tx_timeout = &BoltProtocolV3_set_begin_tx_timeout;
    protocol->set_begin_tx_metadata = &BoltProtocolV3_set_begin_tx_metadata;
    protocol->load_begin_tx = &BoltProtocolV3_load_begin_tx;

    protocol->load_commit_tx = &BoltProtocolV3_load_commit_tx;
    protocol->load_rollback_tx = &BoltProtocolV3_load_rollback_tx;
    protocol->load_discard = &BoltProtocolV3_load_discard;
    protocol->load_pull = &BoltProtocolV3_load_pull;
    protocol->load_reset = &BoltProtocolV3_load_reset;

    protocol->last_request = &BoltProtocolV3_last_request;

    protocol->field_names = &BoltProtocolV3_result_field_names;
    protocol->field_values = &BoltProtocolV3_result_field_values;
    protocol->metadata = &BoltProtocolV3_result_metadata;
    protocol->failure = &BoltProtocolV3_failure;

    protocol->last_data_type = &BoltProtocolV3_last_data_type;
    protocol->last_bookmark = &BoltProtocolV3_last_bookmark;
    protocol->server = &BoltProtocolV3_server;

    protocol->is_failure_summary = &BoltProtocolV3_is_failure_summary;
    protocol->is_success_summary = &BoltProtocolV3_is_success_summary;
    protocol->is_ignored_summary = &BoltProtocolV3_is_ignored_summary;

    protocol->fetch = &BoltProtocolV3_fetch;

    return protocol;
}

void BoltProtocolV3_destroy_protocol(struct BoltProtocol* protocol)
{
    if (protocol!=NULL) {
        BoltProtocolV3_destroy_state(protocol->proto_state);
    }

    BoltMem_deallocate(protocol, sizeof(struct BoltProtocol));
}
