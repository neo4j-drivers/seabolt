/*
 * Copyright (c) 2002-2017 "Neo Technology,"
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
#include <connect.h>


#define BOLT_SUCCESS 0x70
#define BOLT_RECORD  0x71
#define BOLT_IGNORED 0x7E
#define BOLT_FAILURE 0x7F


enum BoltProtocolV1Type
{
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

struct _run_request
{
    struct BoltValue* request;
    struct BoltValue* statement;
    struct BoltValue* parameters;
};

struct BoltProtocolV1State
{
    // These buffers exclude chunk headers.
    struct BoltBuffer* tx_buffer;
    struct BoltBuffer* rx_buffer;

    int next_request_id;
    int response_counter;
    unsigned long long record_counter;

    struct _run_request run;
    struct _run_request begin;
    struct _run_request commit;
    struct _run_request rollback;
    struct BoltValue* discard_request;
    struct BoltValue* pull_request;

    /// Holder for fetched data and metadata
    struct BoltValue* data;
};

struct BoltProtocolV1State* BoltProtocolV1_create_state();

void BoltProtocolV1_destroy_state(struct BoltProtocolV1State* state);

struct BoltProtocolV1State* BoltProtocolV1_state(struct BoltConnection* connection);

int BoltProtocolV1_load_message(struct BoltConnection * connection, struct BoltValue * value);

int BoltProtocolV1_load_message_quietly(struct BoltConnection * connection, struct BoltValue * value);

int BoltProtocolV1_compile_INIT(struct BoltValue* value, const char* user_agent, const char* user, const char* password);

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


#endif // SEABOLT_PROTOCOL_V1
