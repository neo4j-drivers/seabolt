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

#include <bolt/connections.h>
#include "bolt/config-impl.h"
#include "bolt/logging.h"
#include "bolt/mem.h"
#include "bolt/pooling.h"

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

int find_unused_connection(struct BoltConnectionPool* pool)
{
    for (size_t i = 0; i<pool->size; i++) {
        struct BoltConnection* connection = &pool->connections[i];
        if (connection->agent==NULL) {
            return (int) i;
        }
    }
    return -1;
}

int find_connection(struct BoltConnectionPool* pool, struct BoltConnection* connection)
{
    for (size_t i = 0; i<pool->size; i++) {
        struct BoltConnection* candidate = &pool->connections[i];
        if (candidate==connection) {
            return (int) i;
        }
    }
    return -1;
}

enum BoltConnectionPoolAcquireStatus init(struct BoltConnectionPool* pool, int index)
{
    struct BoltConnection* connection = &pool->connections[index];
    switch (BoltConnection_init(connection, pool->user_agent, pool->auth_token)) {
    case 0:
        return POOL_NO_ERROR;
    default:
        return POOL_CONNECTION_ERROR;
    }
}

int reset(struct BoltConnectionPool* pool, int index)
{
    struct BoltConnection* connection = &pool->connections[index];
    switch (BoltConnection_load_reset_request(connection)) {
    case 0: {
        bolt_request_t request_id = BoltConnection_last_request(connection);
        if (BoltConnection_send(connection) < 0) {
            return -1;
        }

        if (BoltConnection_fetch_summary(connection, request_id) < 0) {
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

enum BoltConnectionPoolAcquireStatus open_init(struct BoltConnectionPool* pool, int index)
{
    // Host name resolution is carried out every time a connection
    // is opened. Given that connections are pooled and reused,
    // this is not a huge overhead.
    switch (BoltAddress_resolve(pool->address)) {
    case 0:
        break;
    default:
        return POOL_ADDRESS_NOT_RESOLVED;  // Could not resolve address
    }
    struct BoltConnection* connection = &pool->connections[index];
    switch (BoltConnection_open(connection, pool->transport, pool->address)) {
    case 0:
        return init(pool, index);
    default:
        return POOL_CONNECTION_ERROR;  // Could not open socket
    }
}

void close_pool_entry(struct BoltConnectionPool* pool, int index)
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

enum BoltConnectionPoolAcquireStatus reset_or_open_init(struct BoltConnectionPool* pool, int index)
{
    switch (reset(pool, index)) {
    case 0:
        return POOL_NO_ERROR;
    default:
        return open_init(pool, index);
    }
}

void reset_or_close(struct BoltConnectionPool* pool, int index)
{
    // TODO: disconnect if too old
    switch (reset(pool, index)) {
    case 0:
        break;
    default:
        close_pool_entry(pool, index);
    }
}

struct BoltConnectionPool* BoltConnectionPool_create(enum BoltTransport transport, struct BoltAddress* address,
        const char* user_agent, const struct BoltValue* auth_token, uint32_t size)
{
    BoltLog_info("bolt: creating pool");
    struct BoltConnectionPool* pool = BoltMem_allocate(SIZE_OF_CONNECTION_POOL);
    BoltUtil_mutex_create(&pool->mutex);
    pool->transport = transport;
    pool->address = address;
    pool->user_agent = BoltMem_duplicate(user_agent, strlen(user_agent)+1);
    pool->auth_token = BoltValue_duplicate(auth_token);
    pool->size = size;
    pool->connections = BoltMem_allocate(size*sizeof(struct BoltConnection));
    memset(pool->connections, 0, size*sizeof(struct BoltConnection));
    return pool;
}

void BoltConnectionPool_destroy(struct BoltConnectionPool* pool)
{
    BoltLog_info("bolt: destroying pool");
    for (size_t index = 0; index<pool->size; index++) {
        close_pool_entry(pool, (int) index);
    }
    BoltMem_deallocate(pool->connections, pool->size*sizeof(struct BoltConnection));
    BoltUtil_mutex_destroy(&pool->mutex);
    BoltMem_deallocate(pool->user_agent, strlen(pool->user_agent)+1);
    BoltValue_destroy(pool->auth_token);
    BoltMem_deallocate(pool, SIZE_OF_CONNECTION_POOL);
}

struct BoltConnectionPoolAcquireResult BoltConnectionPool_acquire(struct BoltConnectionPool* pool, const void* agent)
{
    BoltLog_info("bolt: acquiring connection from the pool");
    BoltUtil_mutex_lock(&pool->mutex);
    int index = find_unused_connection(pool);
    enum BoltConnectionPoolAcquireStatus pool_status = index>=0 ? POOL_NO_ERROR : POOL_FULL;
    if (index>=0) {
        switch (pool->connections[index].status) {
        case BOLT_DISCONNECTED:
        case BOLT_DEFUNCT:
            // if the connection is DISCONNECTED or DEFUNCT then try
            // to open and initialise it before handing it out.
            pool_status = open_init(pool, index);
            break;
        case BOLT_CONNECTED:
            // If CONNECTED, the connection will need to be initialised.
            // This state should rarely, if ever, be encountered here.
            // TODO: disconnect if too old
            pool_status = init(pool, index);
            break;
        case BOLT_FAILED:
            // If FAILED, attempt to RESET the connection, reopening
            // from scratch if that fails. This state should rarely,
            // if ever, be encountered here.
            // TODO: disconnect if too old
            pool_status = reset_or_open_init(pool, index);
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

    struct BoltConnectionPoolAcquireResult handle;
    handle.status = pool_status;
    handle.connection_error = BOLT_NO_ERROR;
    handle.connection_status = BOLT_DISCONNECTED;
    handle.connection = NULL;

    switch (pool_status) {
    case POOL_NO_ERROR:
        handle.connection = &pool->connections[index];
        handle.connection->agent = agent;
        break;
    case POOL_CONNECTION_ERROR:
        handle.connection_status = pool->connections[index].status;
        handle.connection_error = pool->connections[index].error;
        break;
    case POOL_FULL:
    case POOL_ADDRESS_NOT_RESOLVED:
        break;
    }

    BoltUtil_mutex_unlock(&pool->mutex);

    return handle;
}

int BoltConnectionPool_release(struct BoltConnectionPool* pool, struct BoltConnection* connection)
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
