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

/**
 * @file
 */

#ifndef SEABOLT_POOLING_H
#define SEABOLT_POOLING_H

#include "communication-secure.h"
#include "connector.h"
#include "sync.h"

typedef struct BoltDirectPool {
    mutex_t mutex;
    cond_t released_cond;
    char* id;
    struct BoltAddress* address;
    const struct BoltValue* auth_token;
    const struct BoltConfig* config;
    BoltSecurityContext* sec_context;
    int max_size;
    volatile int64_t in_use_count;
    BoltConnection** connections;
} BoltDirectPool;

#define SIZE_OF_DIRECT_POOL sizeof(struct BoltDirectPool)
#define SIZE_OF_DIRECT_POOL_PTR sizeof(struct BoltDirectPool*)

struct BoltDirectPool*
BoltDirectPool_create(const struct BoltAddress* address, const struct BoltValue* auth_token,
        const struct BoltConfig* config);

void BoltDirectPool_destroy(struct BoltDirectPool* pool);

BoltConnection* BoltDirectPool_acquire(struct BoltDirectPool* pool, BoltStatus* status);

int BoltDirectPool_release(struct BoltDirectPool* pool, struct BoltConnection* connection);

int BoltDirectPool_connections_in_use(struct BoltDirectPool* pool);

#endif //SEABOLT_POOLING_H
