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

#ifndef SEABOLT_PROTOCOL_V1
#define SEABOLT_PROTOCOL_V1

#include "connect.h"
#include "values.h"


typedef enum {
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
} BoltProtocolV1Type;

BoltProtocolV1Type BoltProtocolV1_markerType(uint8_t marker);

int BoltProtocolV1_loadString(BoltConnection* connection, const char* string, int32_t size);

int BoltProtocolV1_loadMap(BoltConnection* connection, BoltValue* value);

int BoltProtocolV1_load(BoltConnection* connection, BoltValue* value);

int BoltProtocolV1_loadInit(BoltConnection* connection, const char* user, const char* password);

int BoltProtocolV1_loadRun(BoltConnection* connection, const char* statement);

int BoltProtocolV1_loadPull(BoltConnection* connection);

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
int BoltProtocolV1_unload(BoltConnection* connection, BoltValue* value);


#endif // SEABOLT_PROTOCOL_V1
