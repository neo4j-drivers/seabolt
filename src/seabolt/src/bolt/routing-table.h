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
#ifndef SEABOLT_ALL_ROUTING_TABLE_H
#define SEABOLT_ALL_ROUTING_TABLE_H

#include "connector.h"
#include "values.h"

struct RoutingTable {
    int64_t expires;
    int64_t last_updated;
    struct BoltAddressSet* readers;
    struct BoltAddressSet* writers;
    struct BoltAddressSet* routers;
};

#define SIZE_OF_ROUTING_TABLE sizeof(struct RoutingTable)
#define SIZE_OF_ROUTING_TABLE_PTR sizeof(struct RoutingTable*)

struct RoutingTable* RoutingTable_create();

void RoutingTable_destroy(struct RoutingTable* state);

int RoutingTable_update(struct RoutingTable* state, struct BoltValue* response);

int RoutingTable_is_expired(struct RoutingTable* state, BoltAccessMode mode);

void RoutingTable_forget_server(struct RoutingTable* state, const struct BoltAddress* address);

void RoutingTable_forget_writer(struct RoutingTable* state, const struct BoltAddress* address);

#endif //SEABOLT_ALL_ROUTING_TABLE_H
