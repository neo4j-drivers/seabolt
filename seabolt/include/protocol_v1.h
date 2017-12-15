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
} ;

enum BoltProtocolV1Type BoltProtocolV1_marker_type(uint8_t marker);

int BoltProtocolV1_load_null(struct BoltConnection* connection);

int BoltProtocolV1_load_boolean(struct BoltConnection* connection, int value);

int BoltProtocolV1_load_integer(struct BoltConnection* connection, int64_t value);

int BoltProtocolV1_load_float(struct BoltConnection* connection, double value);

int BoltProtocolV1_load_bytes(struct BoltConnection* connection, const char* string, int32_t size);

int BoltProtocolV1_load_string(struct BoltConnection* connection, const char* string, int32_t size);

int BoltProtocolV1_load(struct BoltConnection* connection, struct BoltValue* value);

int BoltProtocolV1_Value_to_INIT(struct BoltValue* value, const char* user_agent, const char* user, const char* password);

int BoltProtocolV1_Value_to_RUN(struct BoltValue* value, const char* statement);

int BoltProtocolV1_Value_to_DISCARD_ALL(struct BoltValue* value);

int BoltProtocolV1_Value_to_PULL_ALL(struct BoltValue* value);

/**
 * Top-level unload.
 *
 * For a typical Bolt v1 data stream, this will unload either a summary
 * or the first value of a record.
 *
 * @param connection
 * @param value
 * @return
 */
int BoltProtocolV1_unload(struct BoltConnection* connection, struct BoltValue* value);

const char* BoltProtocolV1_structure_name(int16_t code);

const char* BoltProtocolV1_request_name(int16_t code);

const char* BoltProtocolV1_summary_name(int16_t code);


#endif // SEABOLT_PROTOCOL_V1
