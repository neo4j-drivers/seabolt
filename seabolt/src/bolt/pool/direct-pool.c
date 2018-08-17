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

#include <bolt/connections.h>
#include "bolt/config-impl.h"
#include "bolt/logging.h"
#include "bolt/mem.h"
#include "direct-pool.h"

#define SIZE_OF_CONNECTION_POOL sizeof(struct BoltConnectionPool)

// TODO: put this somewhere else
void timespec_diff(struct timespec* t, struct timespec* t0, struct timespec* t1)
{
    t->tv_sec = t0->tv_sec-t1->tv_sec;
    t->tv_nsec = t0->tv_nsec-t1->tv_nsec;
    while (t->tv_nsec>=1000000000) {
        t->tv_sec += 1;
        t->tv_nsec -= 1000000000;
    }
    while (t->tv_nsec<0) {
        t->tv_sec -= 1;
        t->tv_nsec += 1000000000;
    }
}

int find_unused_connection(struct BoltDirectPool* pool)
{
    for (size_t i = 0; i<pool->size; i++) {
        struct BoltConnection* connection = &pool->connections[i];
        if (connection->agent==NULL) {
            return (int) i;
        }
    }
    return -1;
}

int find_connection(struct BoltDirectPool* pool, struct BoltConnection* connection)
{
    for (size_t i = 0; i<pool->size; i++) {
        struct BoltConnection* candidate = &pool->connections[i];
        if (candidate==connection) {
            return (int) i;
        }
    }
    return -1;
}

enum BoltConnectionError init(struct BoltDirectPool* pool, int index)
{
    struct BoltConnection* connection = &pool->connections[index];
    switch (BoltConnection_init(connection, pool->config->user_agent, pool->config->auth_token)) {
    case 0:
        return BOLT_SUCCESS;
    default:
        return BOLT_CONNECTION_HAS_MORE_INFO;
    }
}

int reset(struct BoltDirectPool* pool, int index)
{
    struct BoltConnection* connection = &pool->connections[index];
    switch (BoltConnection_load_reset_request(connection)) {
    case 0: {
        bolt_request_t request_id = BoltConnection_last_request(connection);
        if (BoltConnection_send(connection)<0) {
            return -1;
        }

        if (BoltConnection_fetch_summary(connection, request_id)<0) {
            return -1;
        }

        if (BoltConnection_summary_success(connection)) {
            return 0;
        }

        return -1;
    }
    default:
        return -1;
    }
}

enum BoltConnectionError open_init(struct BoltDirectPool* pool, int index)
{
    // Host name resolution is carried out every time a connection
    // is opened. Given that connections are pooled and reused,
    // this is not a huge overhead.
    switch (BoltAddress_resolve(pool->address)) {
    case 0:
        break;
    default:
        return BOLT_ADDRESS_NOT_RESOLVED;  // Could not resolve address
    }
    struct BoltConnection* connection = &pool->connections[index];
    switch (BoltConnection_open(connection, pool->config->transport, pool->address)) {
    case 0:
        return init(pool, index);
    default:
        return BOLT_CONNECTION_HAS_MORE_INFO;  // Could not open socket
    }
}

void close_pool_entry(struct BoltDirectPool* pool, int index)
{
    struct BoltConnection* connection = &pool->connections[index];
    if (connection->status!=BOLT_DISCONNECTED) {
        struct timespec now;
        struct timespec diff;
        BoltUtil_get_time(&now);
        timespec_diff(&diff, &now, &connection->metrics.time_opened);
        BoltLog_info("bolt: Connection alive for %lds %09ldns", (long) (diff.tv_sec), diff.tv_nsec);
        BoltConnection_close(connection);
    }
}

enum BoltConnectionError reset_or_open_init(struct BoltDirectPool* pool, int index)
{
    switch (reset(pool, index)) {
    case 0:
        return BOLT_SUCCESS;
    default:
        return open_init(pool, index);
    }
}

void reset_or_close(struct BoltDirectPool* pool, int index)
{
    // TODO: disconnect if too old
    switch (reset(pool, index)) {
    case 0:
        break;
    default:
        close_pool_entry(pool, index);
    }
}

struct BoltDirectPool* BoltDirectPool_create(struct BoltAddress* address, struct BoltConfig* config)
{
    BoltLog_info("bolt: creating pool");
    struct BoltDirectPool* pool = (struct BoltDirectPool*) BoltMem_allocate(SIZE_OF_DIRECT_POOL);
    BoltUtil_mutex_create(&pool->mutex);
    pool->config = config;
    pool->address = BoltAddress_create(address->host, address->port);
    pool->size = config->max_pool_size;
    pool->connections = (struct BoltConnection*) BoltMem_allocate(config->max_pool_size*sizeof(struct BoltConnection));
    memset(pool->connections, 0, config->max_pool_size*sizeof(struct BoltConnection));
    return pool;
}

void BoltDirectPool_destroy(struct BoltDirectPool* pool)
{
    BoltLog_info("bolt: destroying pool");
    for (size_t index = 0; index<pool->size; index++) {
        close_pool_entry(pool, (int) index);
    }
    BoltMem_deallocate(pool->connections, pool->size*sizeof(struct BoltConnection));
    BoltAddress_destroy(pool->address);
    BoltUtil_mutex_destroy(&pool->mutex);
    BoltMem_deallocate(pool, SIZE_OF_DIRECT_POOL);
}

struct BoltConnectionResult BoltDirectPool_acquire(struct BoltDirectPool* pool)
{
    BoltLog_info("bolt: acquiring connection from the pool");
    BoltUtil_mutex_lock(&pool->mutex);
    int index = find_unused_connection(pool);
    enum BoltConnectionError pool_error = index>=0 ? BOLT_SUCCESS : BOLT_POOL_FULL;
    if (index>=0) {
        switch (pool->connections[index].status) {
        case BOLT_DISCONNECTED:
        case BOLT_DEFUNCT:
            // if the connection is DISCONNECTED or DEFUNCT then try
            // to open and initialise it before handing it out.
            pool_error = open_init(pool, index);
            break;
        case BOLT_CONNECTED:
            // If CONNECTED, the connection will need to be initialised.
            // This state should rarely, if ever, be encountered here.
            // TODO: disconnect if too old
            pool_error = init(pool, index);
            break;
        case BOLT_FAILED:
            // If FAILED, attempt to RESET the connection, reopening
            // from scratch if that fails. This state should rarely,
            // if ever, be encountered here.
            // TODO: disconnect if too old
            pool_error = reset_or_open_init(pool, index);
            break;
        case BOLT_READY:
            // If the connection is already in the READY state then
            // do nothing and assume that the connection hasn't been
            // timed out by some piece of network housekeeping
            // infrastructure. Such timeouts should instead be managed
            // by setting the maximum connection lifetime.
            // TODO: disconnect if too old
            break;
        }
    }

    struct BoltConnectionResult handle;
    handle.connection_status = BOLT_DISCONNECTED;
    handle.connection_error = BOLT_SUCCESS;
    handle.connection_error_ctx = NULL;
    handle.connection = NULL;

    switch (pool_error) {
    case BOLT_SUCCESS:
        handle.connection = &pool->connections[index];
        handle.connection->agent = "USED";
        handle.connection_status = handle.connection->status;
        break;
    case BOLT_CONNECTION_HAS_MORE_INFO:
        handle.connection_status = pool->connections[index].status;
        handle.connection_error = pool->connections[index].error;
        handle.connection_error_ctx = pool->connections[index].error_ctx;
        break;
    default:
        handle.connection_status = BOLT_DISCONNECTED;
        handle.connection_error = pool_error;
        handle.connection_error_ctx = NULL;
        break;
    }

    BoltUtil_mutex_unlock(&pool->mutex);

    return handle;
}

int BoltDirectPool_release(struct BoltDirectPool* pool, struct BoltConnection* connection)
{
    BoltLog_info("bolt: releasing connection to pool");
    BoltUtil_mutex_lock(&pool->mutex);
    int index = find_connection(pool, connection);
    if (index>=0) {
        connection->agent = NULL;
        reset_or_close(pool, index);
    }
    BoltUtil_mutex_unlock(&pool->mutex);
    return index;
}

int BoltDirectPool_connections_in_use(struct BoltDirectPool* pool)
{
    int count = 0;
    for (uint32_t i = 0; i<pool->size; i++) {
        if (pool->connections[i].agent!=NULL) {
            count++;
        }
    }
    return count;
}