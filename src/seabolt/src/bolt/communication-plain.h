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
#ifndef SEABOLT_COMMUNICATION_PLAIN_H
#define SEABOLT_COMMUNICATION_PLAIN_H

#include "communication.h"
#include "config.h"

typedef struct PlainCommunicationContext {
    BoltAddress* local_endpoint;
    BoltAddress* remote_endpoint;
    int fd_socket;

    void* action_to_restore;
} PlainCommunicationContext;

BoltCommunication* BoltCommunication_create_plain(BoltSocketOptions* socket_options, BoltLog* log);

#endif //SEABOLT_COMMUNICATION_PLAIN_H
