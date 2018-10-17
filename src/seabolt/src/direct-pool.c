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

#include "config-impl.h"
#include "connections.h"
#include "logging.h"
#include "mem.h"
#include "tls.h"
#include "utils.h"
#include "direct-pool.h"
#include "protocol.h"

void close_pool_entry(struct BoltDirectPool* pool, int index)
{
    struct BoltConnection* connection = &pool->connections[index];
    if (connection->status!=BOLT_DISCONNECTED) {
        if (connection->metrics.time_opened.tv_sec!=0 || connection->metrics.time_opened.tv_nsec!=0) {
            struct timespec now;
            struct timespec diff;
            BoltUtil_get_time(&now);
            BoltUtil_diff_time(&diff, &now, &connection->metrics.time_opened);
            BoltLog_info(pool->config->log, "Connection alive for %lds %09ldns", (long) (diff.tv_sec),
                    diff.tv_nsec);
        }

        BoltConnection_close(connection);
    }
}

int find_unused_connection(struct BoltDirectPool* pool)
{
    for (int i = 0; i<pool->size; i++) {
        struct BoltConnection* connection = &pool->connections[i];

        if (connection->agent==NULL) {
            // check for max lifetime and close existing connection if required
            if (connection->status!=BOLT_DISCONNECTED && connection->status!=BOLT_DEFUNCT) {
                if (pool->config->max_connection_lifetime>0) {
                    int64_t now = BoltUtil_get_time_ms();
                    int64_t created = BoltUtil_get_time_ms_from(&connection->metrics.time_opened);

                    if (now-created>pool->config->max_connection_lifetime) {
                        BoltLog_info(pool->config->log, "Connection reached its maximum lifetime, force closing.");

                        close_pool_entry(pool, i);
                    }
                }
            }

            return i;
        }
    }
    return -1;
}

int find_connection(struct BoltDirectPool* pool, struct BoltConnection* connection)
{
    for (int i = 0; i<pool->size; i++) {
        struct BoltConnection* candidate = &pool->connections[i];
        if (candidate==connection) {
            return i;
        }
    }
    return -1;
}

int init(struct BoltDirectPool* pool, int index)
{
    struct BoltConnection* connection = &pool->connections[index];
    switch (BoltConnection_init(connection, pool->config->user_agent, pool->auth_token)) {
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
        bolt_request request_id = BoltConnection_last_request(connection);
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

int open_init(struct BoltDirectPool* pool, int index)
{
    // Host name resolution is carried out every time a connection
    // is opened. Given that connections are pooled and reused,
    // this is not a huge overhead.
    switch (BoltAddress_resolve(pool->address, pool->config->log)) {
    case 0:
        break;
    default:
        return BOLT_ADDRESS_NOT_RESOLVED;  // Could not resolve address
    }
    struct BoltConnection* connection = &pool->connections[index];
    switch (BoltConnection_open(connection, pool->config->transport, pool->address, pool->config->trust,
            pool->config->log, pool->config->sock_opts)) {
    case 0:
        return init(pool, index);
    default:
        return BOLT_CONNECTION_HAS_MORE_INFO;  // Could not open socket
    }
}

int reset_or_open_init(struct BoltDirectPool* pool, int index)
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
    switch (reset(pool, index)) {
    case 0:
        break;
    default:
        close_pool_entry(pool, index);
    }
}

struct BoltDirectPool* BoltDirectPool_create(const struct BoltAddress* address, const struct BoltValue* auth_token,
        const struct BoltConfig* config)
{
    BoltLog_info(config->log, "creating pool");
    struct BoltDirectPool* pool = (struct BoltDirectPool*) BoltMem_allocate(SIZE_OF_DIRECT_POOL);
    BoltUtil_mutex_create(&pool->mutex);
    pool->config = config;
    pool->address = BoltAddress_create(address->host, address->port);
    pool->auth_token = auth_token;
    pool->size = config->max_pool_size;
    pool->connections = (struct BoltConnection*) BoltMem_allocate(config->max_pool_size*sizeof(struct BoltConnection));
    memset(pool->connections, 0, config->max_pool_size*sizeof(struct BoltConnection));
    if (config->transport==BOLT_SECURE_SOCKET) {
        pool->ssl_context = create_ssl_ctx(config->trust, address->host, config->log);

        // assign ssl_context to all connections
        for (int i = 0; i<config->max_pool_size; i++) {
            pool->connections[0].ssl_context = pool->ssl_context;
            pool->connections[0].owns_ssl_context = 0;
        }
    }
    else {
        pool->ssl_context = NULL;
    }
    return pool;
}

void BoltDirectPool_destroy(struct BoltDirectPool* pool)
{
    BoltLog_info(pool->config->log, "destroying pool");
    for (int index = 0; index<pool->size; index++) {
        close_pool_entry(pool, index);
    }
    BoltMem_deallocate(pool->connections, pool->size*sizeof(struct BoltConnection));
    if (pool->ssl_context!=NULL) {
        free_ssl_context(pool->ssl_context);
    }
    BoltAddress_destroy(pool->address);
    BoltUtil_mutex_destroy(&pool->mutex);
    BoltMem_deallocate(pool, SIZE_OF_DIRECT_POOL);
}

struct BoltConnectionResult BoltDirectPool_acquire(struct BoltDirectPool* pool)
{
    struct BoltConnectionResult handle;
    int index = 0;
    int pool_error = BOLT_SUCCESS;

    int64_t started_at = BoltUtil_get_time_ms();
    BoltLog_info(pool->config->log, "acquiring connection from the pool");

    while (1) {
        BoltUtil_mutex_lock(&pool->mutex);

        index = find_unused_connection(pool);
        pool_error = index>=0 ? BOLT_SUCCESS : BOLT_POOL_FULL;
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
                pool_error = init(pool, index);
                break;
            case BOLT_FAILED:
                // If FAILED, attempt to RESET the connection, reopening
                // from scratch if that fails. This state should rarely,
                // if ever, be encountered here.
                pool_error = reset_or_open_init(pool, index);
                break;
            case BOLT_READY:
                // If the connection is already in the READY state then
                // do nothing and assume that the connection hasn't been
                // timed out by some piece of network housekeeping
                // infrastructure. Such timeouts should instead be managed
                // by setting the maximum connection lifetime.
                break;
            }
        }

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

        if (pool->config->max_connection_acquire_time==0) {
            // fail fast, we report the failure asap
            break;
        }

        // Retry acquire operation until we get a live connection or timeout
        if (handle.connection_error==BOLT_POOL_FULL) {
            if (pool->config->max_connection_acquire_time>0
                    && BoltUtil_get_time_ms()-started_at>pool->config->max_connection_acquire_time) {
                handle.connection_status = BOLT_DISCONNECTED;
                handle.connection_error = BOLT_POOL_ACQUISITION_TIMED_OUT;
                handle.connection_error_ctx = NULL;

                break;
            }
            else {
                BoltLog_info(pool->config->log, "Pool is full, will retry acquiring a connection from the pool.");

                BoltUtil_sleep(250);

                continue;
            }
        }
        else {
            break;
        }
    }

    return handle;
}

int BoltDirectPool_release(struct BoltDirectPool* pool, struct BoltConnection* connection)
{
    BoltLog_info(pool->config->log, "releasing connection to pool");
    BoltUtil_mutex_lock(&pool->mutex);
    int index = find_connection(pool, connection);
    if (index>=0) {
        connection->agent = NULL;

        connection->protocol->clear_run(connection);
        connection->protocol->clear_begin_tx(connection);

        reset_or_close(pool, index);
    }
    BoltUtil_mutex_unlock(&pool->mutex);
    return index;
}

int BoltDirectPool_connections_in_use(struct BoltDirectPool* pool)
{
    int count = 0;
    for (int i = 0; i<pool->size; i++) {
        if (pool->connections[i].agent!=NULL) {
            count++;
        }
    }
    return count;
}

