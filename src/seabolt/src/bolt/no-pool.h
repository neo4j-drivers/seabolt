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

#ifndef SEABOLT_NO_POOL_H
#define SEABOLT_NO_POOL_H

#include "communication-secure.h"
#include "connector.h"
#include "sync.h"

/**
 * Pooling contract for a no-pooling connection acquisition
 */
struct BoltNoPool {
    mutex_t mutex;
    char* id;
    struct BoltAddress* address;
    const struct BoltValue* auth_token;
    const struct BoltConfig* config;
    volatile int size;
    volatile BoltConnection** connections;
};

#define SIZE_OF_NO_POOL sizeof(struct BoltNoPool)
#define SIZE_OF_NO_POOL_PTR sizeof(struct BoltNoPool*)

struct BoltNoPool*
BoltNoPool_create(const struct BoltAddress* address, const struct BoltValue* auth_token,
        const struct BoltConfig* config);

void BoltNoPool_destroy(struct BoltNoPool* pool);

BoltConnection* BoltNoPool_acquire(struct BoltNoPool* pool, BoltStatus* status);

int BoltNoPool_release(struct BoltNoPool* pool, struct BoltConnection* connection);

#endif //SEABOLT_POOLING_H
