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

int BoltProtocolV1_load_string(struct BoltConnection* connection, const char* string, int32_t size);

int BoltProtocolV1_load_map(struct BoltConnection* connection, struct BoltValue* value);

int BoltProtocolV1_load(struct BoltConnection* connection, struct BoltValue* value);

int BoltProtocolV1_load_init(struct BoltConnection* connection, const char* user, const char* password);

int BoltProtocolV1_load_run(struct BoltConnection* connection, const char* statement);

int BoltProtocolV1_load_pull(struct BoltConnection* connection);

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


#endif // SEABOLT_PROTOCOL_V1
