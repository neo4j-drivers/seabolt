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

/**
 * @file
 */

#ifndef SEABOLT_PROTOCOL_V1
#define SEABOLT_PROTOCOL_V1

#include <stdint.h>

#include "bolt/connections.h"
#include "protocol.h"

#define BOLT_V1_SUCCESS 0x70
#define BOLT_V1_RECORD  0x71
#define BOLT_V1_IGNORED 0x7E
#define BOLT_V1_FAILURE 0x7F

#define BOLT_MAX_CHUNK_SIZE 65535

struct _run_request {
    struct BoltMessage* request;
    struct BoltValue* statement;
    struct BoltValue* parameters;
};

struct BoltMessage {
    int8_t code;
    struct BoltValue* fields;
};

struct BoltProtocolV1State {
    check_struct_signature_func check_readable_struct;
    check_struct_signature_func check_writable_struct;

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

    bolt_request_t next_request_id;
    bolt_request_t response_counter;
    unsigned long long record_counter;

    struct _run_request run;
    struct _run_request begin;
    struct _run_request commit;
    struct _run_request rollback;

    struct BoltMessage* discard_request;
    struct BoltMessage* pull_request;
    struct BoltMessage* reset_request;

    /// Holder for fetched data and metadata
    int16_t data_type;
    struct BoltValue* data;

};

int BoltProtocolV1_check_readable_struct_signature(int16_t signature);

int BoltProtocolV1_check_writable_struct_signature(int16_t signature);

struct BoltProtocolV1State* BoltProtocolV1_create_state();

void BoltProtocolV1_destroy_state(struct BoltProtocolV1State* state);

struct BoltProtocolV1State* BoltProtocolV1_state(struct BoltConnection* connection);

int BoltProtocolV1_load_message(struct BoltConnection* connection, struct BoltMessage* message, int quiet);

int BoltProtocolV1_compile_INIT(struct BoltMessage* message, const char* user_agent, const struct BoltValue* auth_token,
        int mask_secure_fields);

int BoltProtocolV1_fetch(struct BoltConnection* connection, bolt_request_t request_id);

/**
 * Top-level unload.
 *
 * For a typical Bolt v1 data stream, this will unload either a summary
 * or the first value of a record.
 *
 * @param connection
 * @return
 */
int BoltProtocolV1_unload(struct BoltConnection* connection);

const char* BoltProtocolV1_structure_name(int16_t code);

const char* BoltProtocolV1_message_name(int16_t code);

int BoltProtocolV1_init(struct BoltConnection* connection, const char* user_agent, const struct BoltValue* auth_token);

void BoltProtocolV1_clear_failure(struct BoltConnection* connection);

void BoltProtocolV1_extract_metadata(struct BoltConnection* connection, struct BoltValue* summary);

int BoltProtocolV1_set_cypher_template(struct BoltConnection* connection, const char* statement, size_t size);

int BoltProtocolV1_set_n_cypher_parameters(struct BoltConnection* connection, int32_t size);

int BoltProtocolV1_set_cypher_parameter_key(struct BoltConnection* connection, int32_t index, const char* key,
        size_t key_size);

struct BoltValue* BoltProtocolV1_cypher_parameter_value(struct BoltConnection* connection, int32_t index);

int BoltProtocolV1_load_bookmark(struct BoltConnection* connection, const char* bookmark);

int BoltProtocolV1_load_begin_request(struct BoltConnection* connection);

int BoltProtocolV1_load_commit_request(struct BoltConnection* connection);

int BoltProtocolV1_load_rollback_request(struct BoltConnection* connection);

int BoltProtocolV1_load_run_request(struct BoltConnection* connection);

int BoltProtocolV1_load_pull_request(struct BoltConnection* connection, int32_t n);

int BoltProtocolV1_load_reset_request(struct BoltConnection* connection);

struct BoltValue* BoltProtocolV1_result_fields(struct BoltConnection* connection);

struct BoltValue* BoltProtocolV1_result_metadata(struct BoltConnection* connection);

#endif // SEABOLT_PROTOCOL_V1
