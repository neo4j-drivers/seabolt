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
#ifndef SEABOLT_ALL_CONNECTOR_H
#define SEABOLT_ALL_CONNECTOR_H

#include "bolt-public.h"
#include "log.h"
#include "connection.h"
#include "address-resolver.h"

typedef int BoltAccessMode;
#define BOLT_ACCESS_MODE_WRITE  0
#define BOLT_ACCESS_MODE_READ   1

typedef struct BoltConnector BoltConnector;

SEABOLT_EXPORT BoltConnector*
BoltConnector_create(struct BoltAddress* address, struct BoltValue* auth_token, struct BoltConfig* config);

SEABOLT_EXPORT void BoltConnector_destroy(BoltConnector* connector);

SEABOLT_EXPORT BoltConnection* BoltConnector_acquire(BoltConnector* connector, BoltAccessMode mode, BoltStatus *status);

SEABOLT_EXPORT void BoltConnector_release(BoltConnector* connector, struct BoltConnection* connection);

#endif //SEABOLT_ALL_CONNECTOR_H
