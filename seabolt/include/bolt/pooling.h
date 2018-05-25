/*
 * Copyright (c) 2002-2018 "Neo Technology,"
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

#ifndef SEABOLT_POOLING_H
#define SEABOLT_POOLING_H

#include "connections.h"
#include "platform.h"

/**
 * Connection pool (experimental)
 */
struct BoltConnectionPool
{
    mutex_t mutex;
    enum BoltTransport transport;
    struct BoltAddress * address;
    const struct BoltUserProfile * profile;
    size_t size;
    struct BoltConnection * connections;
};


PUBLIC
struct BoltConnectionPool *
BoltConnectionPool_create(enum BoltTransport transport, struct BoltAddress * address, const struct BoltUserProfile * profile, size_t size);

PUBLIC void BoltConnectionPool_destroy(struct BoltConnectionPool * pool);

PUBLIC struct BoltConnection * BoltConnectionPool_acquire(struct BoltConnectionPool * pool, const void * agent);

PUBLIC int BoltConnectionPool_release(struct BoltConnectionPool * pool, struct BoltConnection * connection);


#endif //SEABOLT_POOLING_H
