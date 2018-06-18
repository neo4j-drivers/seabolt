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

/**
 * @file
 */

#ifndef SEABOLT_PROTOCOL_V1
#define SEABOLT_PROTOCOL_V1

#include <stdint.h>

#include "bolt/connections.h"

#define BOLT_V1_SUCCESS 0x70
#define BOLT_V1_RECORD  0x71
#define BOLT_V1_IGNORED 0x7E
#define BOLT_V1_FAILURE 0x7F

enum BoltProtocolV1Type {
    BOLT_V1_NULL,
    BOLT_V1_BOOLEAN,
    BOLT_V1_INTEGER,
    BOLT_V1_FLOAT,
    BOLT_V1_STRING,
    BOLT_V1_BYTES,
    BOLT_V1_LIST,
    BOLT_V1_MAP,
    BOLT_V1_STRUCTURE,
    BOLT_V1_RESERVED,
};

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
    // These buffers exclude chunk headers.
    struct BoltBuffer* tx_buffer;
    struct BoltBuffer* rx_buffer;

    /// The product name and version of the remote server
    char* server;
    /// A BoltValue containing field names for the active result
    struct BoltValue* result_field_names;
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

    struct BoltMessage* ackfailure_request;
    struct BoltMessage* discard_request;
    struct BoltMessage* pull_request;
    struct BoltMessage* reset_request;

    /// Holder for fetched data and metadata
    int16_t data_type;
    struct BoltValue* data;

};

struct BoltProtocolV1State* BoltProtocolV1_create_state();

void BoltProtocolV1_destroy_state(struct BoltProtocolV1State* state);

struct BoltProtocolV1State* BoltProtocolV1_state(struct BoltConnection* connection);

int BoltProtocolV1_load_message(struct BoltConnection* connection, struct BoltMessage* message);

int BoltProtocolV1_load_message_quietly(struct BoltConnection* connection, struct BoltMessage* message);

int BoltProtocolV1_compile_INIT(struct BoltMessage* message, const struct BoltUserProfile* profile);

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

int BoltProtocolV1_init(struct BoltConnection* connection, const struct BoltUserProfile* profile);

int BoltProtocolV1_reset(struct BoltConnection* connection);

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

int BoltProtocolV1_load_ack_failure(struct BoltConnection* connection);

struct BoltValue* BoltProtocolV1_result_fields(struct BoltConnection* connection);

#endif // SEABOLT_PROTOCOL_V1
