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


int BoltProtocolV1_loadString(BoltConnection* connection, const char* string, int32_t size);

int BoltProtocolV1_loadMap(BoltConnection* connection, BoltValue* value);

int BoltProtocolV1_load(BoltConnection* connection, BoltValue* value);

int BoltProtocolV1_loadInit(BoltConnection* connection, const char* user, const char* password);

int BoltProtocolV1_unload(BoltConnection* connection, BoltValue* value);

int BoltProtocolV1_unloadString(BoltConnection* connection, BoltValue* value);

int BoltProtocolV1_unloadMap(BoltConnection* connection, BoltValue* value);

int BoltProtocolV1_unloadSummary(BoltConnection* connection, BoltValue* value);

#endif // SEABOLT_PROTOCOL_V1
