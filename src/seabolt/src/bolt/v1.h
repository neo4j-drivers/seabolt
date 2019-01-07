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

/**
 * @file
 */

#ifndef SEABOLT_PROTOCOL_V1
#define SEABOLT_PROTOCOL_V1

#include <stdint.h>

#include "connection.h"
#include "protocol.h"

#define BOLT_V1_INIT        0x01
#define BOLT_V1_ACK_FAILURE 0x0E
#define BOLT_V1_RESET       0x0F
#define BOLT_V1_RUN         0x10
#define BOLT_V1_DISCARD_ALL 0x2F
#define BOLT_V1_PULL_ALL    0x3F

#define BOLT_V1_NODE    'N'
#define BOLT_V1_RELATIONSHIP 'R'
#define BOLT_V1_UNBOUND_RELATIONSHIP 'r'
#define BOLT_V1_PATH 'P'

#define BOLT_V1_SUCCESS 0x70
#define BOLT_V1_RECORD  0x71
#define BOLT_V1_IGNORED 0x7E
#define BOLT_V1_FAILURE 0x7F

struct BoltProtocolV1State {
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

    BoltRequest next_request_id;
    BoltRequest response_counter;
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

int BoltProtocolV1_check_readable_struct_signature(int16_t signature);

int BoltProtocolV1_check_writable_struct_signature(int16_t signature);

const char* BoltProtocolV1_structure_name(int16_t code);

struct BoltProtocolV1State* BoltProtocolV1_state(struct BoltConnection* connection);

struct BoltProtocol* BoltProtocolV1_create_protocol();

void BoltProtocolV1_destroy_protocol(struct BoltProtocol* protocol);

#endif // SEABOLT_PROTOCOL_V1
