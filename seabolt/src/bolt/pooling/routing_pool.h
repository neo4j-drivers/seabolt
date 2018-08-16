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
#ifndef SEABOLT_ALL_DISCOVERY_H
#define SEABOLT_ALL_DISCOVERY_H

#include "bolt/connector.h"
#include "bolt/addressing.h"
#include "bolt/values.h"

struct BoltAddressSet {
    int size;
    struct BoltAddress** elements;
};

struct BoltAddressSet* BoltAddressSet_create();

void BoltAddressSet_destroy(struct BoltAddressSet* set);

int BoltAddressSet_size(struct BoltAddressSet* set);

int BoltAddressSet_add(struct BoltAddressSet* set, struct BoltAddress address);

int BoltAddressSet_remove(struct BoltAddressSet* set, struct BoltAddress address);

void BoltAddressSet_replace(struct BoltAddressSet* set, struct BoltAddressSet* others);

void BoltAddressSet_add_all(struct BoltAddressSet* set, struct BoltAddressSet* others);

struct RoutingTable {
    int64_t expires;
    int64_t last_updated;
    struct BoltAddressSet* initial_routers;
    struct BoltAddressSet* readers;
    struct BoltAddressSet* writers;
    struct BoltAddressSet* routers;
};

struct RoutingTable* RoutingTable_create();

struct RoutingTable* RoutingTable_destroy(struct RoutingTable* state);

int RoutingTable_update(struct RoutingTable* state, struct BoltValue* response);

int RoutingTable_is_expired(struct RoutingTable* state, enum BoltAccessMode mode);

struct BoltRoutingConnectionPool {
    struct BoltConfig* config;

    struct RoutingTable* routing_table;
    int readers_offset;
    int writers_offset;

    int size;
    struct BoltAddress** servers;
    struct BoltConnectionPool** server_pools;

    mutex_t lock;
};

struct BoltRoutingConnectionPool*
BoltRoutingConnectionPool_create(struct BoltAddress* address, struct BoltConfig* config);

void BoltRoutingConnectionPool_destroy(struct BoltRoutingConnectionPool* pool);

struct BoltConnectionResult
BoltRoutingConnectionPool_acquire(struct BoltRoutingConnectionPool* pool, enum BoltAccessMode mode);

int BoltRoutingConnectionPool_release(struct BoltRoutingConnectionPool* pool, struct BoltConnection* connection);

#endif //SEABOLT_ALL_DISCOVERY_H
