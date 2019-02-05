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
#ifndef SEABOLT_COMMUNICATION_SECURE_H
#define SEABOLT_COMMUNICATION_SECURE_H

#include "communication.h"

typedef struct BoltSecurityContext BoltSecurityContext;

int BoltSecurityContext_startup();

int BoltSecurityContext_shutdown();

BoltSecurityContext*
BoltSecurityContext_create(BoltTrust* trust, const char* hostname, const BoltLog* log, const char* id);

void BoltSecurityContext_destroy(BoltSecurityContext* context);

BoltCommunication* BoltCommunication_create_secure(BoltSecurityContext* sec_ctx, BoltTrust* trust,
        BoltSocketOptions* socket_options, BoltLog* log, const char* hostname, const char* id);

#endif //SEABOLT_COMMUNICATION_SECURE_H
