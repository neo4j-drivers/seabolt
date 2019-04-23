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

#include "bolt-private.h"
#include "address-private.h"
#include "atomic.h"
#include "config-private.h"
#include "connection-private.h"
#include "no-pool.h"
#include "log-private.h"
#include "mem.h"
#include "protocol.h"
#include "sync.h"
#include "time.h"

#define MAX_ID_LEN 16

static int64_t pool_seq = 0;

int find_open_connection(struct BoltNoPool* pool, struct BoltConnection* connection)
{
    for (int i = 0; i<pool->size; i++) {
        BoltConnection* candidate = (BoltConnection*) pool->connections[i];
        if (candidate==connection) {
            return i;
        }
    }
    return -1;
}

struct BoltNoPool* BoltNoPool_create(const struct BoltAddress* address, const struct BoltValue* auth_token,
        const struct BoltConfig* config)
{
    char* id = BoltMem_allocate(MAX_ID_LEN);
    snprintf(id, MAX_ID_LEN, "pool-%" PRId64, BoltAtomic_increment(&pool_seq));
    BoltLog_info(config->log, "[%s]: Creating pool towards %s:%s", id, address->host, address->port);
    struct BoltNoPool* pool = (struct BoltNoPool*) BoltMem_allocate(SIZE_OF_NO_POOL);
    BoltSync_mutex_create(&pool->mutex);
    pool->id = id;
    pool->config = config;
    pool->address = BoltAddress_create_with_lock(address->host, address->port);
    pool->auth_token = auth_token;
    pool->size = 0;
    pool->connections = NULL;
    return pool;
}

void BoltNoPool_destroy(struct BoltNoPool* pool)
{
    BoltLog_info(pool->config->log, "[%s]: Destroying non-released connections towards %s:%s", pool->id,
            pool->address->host,
            pool->address->port);
    for (int index = 0; index<pool->size; index++) {
        BoltConnection* connection = (BoltConnection*)pool->connections[index];
        BoltConnection_close(connection);
        BoltConnection_destroy(connection);
    }
    BoltMem_deallocate(pool->connections, pool->size*sizeof(BoltConnection*));
    BoltAddress_destroy(pool->address);
    BoltMem_deallocate(pool->id, MAX_ID_LEN);
    BoltSync_mutex_destroy(&pool->mutex);
    BoltMem_deallocate(pool, SIZE_OF_NO_POOL);
}

BoltConnection* BoltNoPool_acquire(struct BoltNoPool* pool, BoltStatus* status)
{
    int pool_error = BOLT_SUCCESS;
    BoltConnection* connection = NULL;

    BoltLog_info(pool->config->log, "[%s]: Acquiring connection towards %s:%s", pool->id,
            pool->address->host, pool->address->port);

    connection = BoltConnection_create();

    switch (BoltAddress_resolve(pool->address, NULL, pool->config->log)) {
    case 0:
        break;
    default:
        pool_error = BOLT_ADDRESS_NOT_RESOLVED;  // Could not resolve address
    }

    if (pool_error==BOLT_SUCCESS) {
        switch (BoltConnection_open(connection, pool->config->transport, pool->address, pool->config->trust,
                pool->config->log, pool->config->socket_options)) {
        case 0:
            break;
        default:
            pool_error = BOLT_CONNECTION_HAS_MORE_INFO;  // Could not open socket
        }
    }

    if (pool_error==BOLT_SUCCESS) {
        switch (BoltConnection_init(connection, pool->config->user_agent, pool->auth_token)) {
        case 0:
            break;
        default:
            pool_error = BOLT_CONNECTION_HAS_MORE_INFO;
        }
    }

    switch (pool_error) {
    case BOLT_SUCCESS: {
        status->state = connection->status->state;
        status->error = BOLT_SUCCESS;
        status->error_ctx = NULL;
        status->error_ctx_size = 0;

        BoltSync_mutex_lock(&pool->mutex);
        int index = pool->size;
        pool->connections = BoltMem_reallocate(pool->connections, pool->size*sizeof(BoltConnection*),
                (pool->size+1)*sizeof(BoltConnection*));
        pool->size = pool->size+1;
        pool->connections[index] = connection;
        BoltSync_mutex_unlock(&pool->mutex);

        break;
    }
    case BOLT_CONNECTION_HAS_MORE_INFO:
        status->state = connection->status->state;
        status->error = connection->status->error;
        status->error_ctx_size = connection->status->error_ctx_size;
        status->error_ctx = BoltMem_duplicate(connection->status->error_ctx, connection->status->error_ctx_size);
        break;
    default:
        status->state = BOLT_CONNECTION_STATE_DISCONNECTED;
        status->error = pool_error;
        status->error_ctx = NULL;
        status->error_ctx_size = 0;
        break;
    }

    return connection;
}

int BoltNoPool_release(struct BoltNoPool* pool, struct BoltConnection* connection)
{
    BoltLog_info(pool->config->log, "[%s]: Closing connection towards %s:%s", pool->id,
            pool->address->host,
            pool->address->port);

    BoltSync_mutex_lock(&pool->mutex);
    int index = find_open_connection(pool, connection);
    if (index>=0) {
        for (int i = index; i<pool->size-1; i++) {
            pool->connections[i] = pool->connections[i+1];
        }
        pool->connections = BoltMem_reallocate(pool->connections, pool->size*sizeof(BoltConnection*),
                (pool->size-1)*sizeof(BoltConnection*));
        pool->size = pool->size-1;
    }
    BoltSync_mutex_unlock(&pool->mutex);

    BoltConnection_close(connection);
    BoltConnection_destroy(connection);

    return index;
}

