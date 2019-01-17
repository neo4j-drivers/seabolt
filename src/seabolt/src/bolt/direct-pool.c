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
#include "direct-pool.h"
#include "log-private.h"
#include "mem.h"
#include "protocol.h"
#include "sync.h"
#include "time.h"
#include "tls.h"

#define MAX_ID_LEN 16

static int64_t pool_seq = 0;

void close_pool_entry(struct BoltDirectPool* pool, int index)
{
    struct BoltConnection* connection = pool->connections[index];
    if (connection->status->state!=BOLT_CONNECTION_STATE_DISCONNECTED) {
        if (connection->metrics->time_opened.tv_sec!=0 || connection->metrics->time_opened.tv_nsec!=0) {
            struct timespec now;
            struct timespec diff;
            BoltTime_get_time(&now);
            BoltTime_diff_time(&diff, &now, &connection->metrics->time_opened);
            BoltLog_info(pool->config->log, "[%s]: Connection alive for %lds %09ldns", BoltConnection_id(connection),
                    (long) (diff.tv_sec), diff.tv_nsec);
        }

        BoltConnection_close(connection);
    }
}

int find_unused_connection(struct BoltDirectPool* pool)
{
    for (int i = 0; i<pool->size; i++) {
        struct BoltConnection* connection = pool->connections[i];

        if (connection->agent==NULL) {
            // check for max lifetime and close existing connection if required
            if (connection->status->state!=BOLT_CONNECTION_STATE_DISCONNECTED
                    && connection->status->state!=BOLT_CONNECTION_STATE_DEFUNCT) {
                if (pool->config->max_connection_life_time>0) {
                    int64_t now = BoltTime_get_time_ms();
                    int64_t created = BoltTime_get_time_ms_from(&connection->metrics->time_opened);

                    if (now-created>pool->config->max_connection_life_time) {
                        BoltLog_info(pool->config->log, "[%s]: Connection reached its maximum lifetime, force closing.",
                                BoltConnection_id(connection));

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
        struct BoltConnection* candidate = pool->connections[i];
        if (candidate==connection) {
            return i;
        }
    }
    return -1;
}

int init(struct BoltDirectPool* pool, int index)
{
    struct BoltConnection* connection = pool->connections[index];
    switch (BoltConnection_init(connection, pool->config->user_agent, pool->auth_token)) {
    case 0:
        return BOLT_SUCCESS;
    default:
        return BOLT_CONNECTION_HAS_MORE_INFO;
    }
}

int reset(struct BoltDirectPool* pool, int index)
{
    struct BoltConnection* connection = pool->connections[index];
    switch (BoltConnection_load_reset_request(connection)) {
    case 0: {
        BoltRequest request_id = BoltConnection_last_request(connection);
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
    switch (BoltAddress_resolve(pool->address, NULL, pool->config->log)) {
    case 0:
        break;
    default:
        return BOLT_ADDRESS_NOT_RESOLVED;  // Could not resolve address
    }
    struct BoltConnection* connection = pool->connections[index];
    switch (BoltConnection_open(connection, pool->config->transport, pool->address, pool->config->trust,
            pool->config->log, pool->config->socket_options)) {
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
    char* id = BoltMem_allocate(MAX_ID_LEN);
    snprintf(id, MAX_ID_LEN, "pool-%" PRId64, BoltAtomic_increment(&pool_seq));
    BoltLog_info(config->log, "[%s]: Creating pool towards %s:%s", id, address->host, address->port);
    struct BoltDirectPool* pool = (struct BoltDirectPool*) BoltMem_allocate(SIZE_OF_DIRECT_POOL);
    BoltSync_mutex_create(&pool->mutex);
    BoltSync_cond_create(&pool->released_cond);
    pool->id = id;
    pool->config = config;
    pool->address = BoltAddress_create(address->host, address->port);
    pool->auth_token = auth_token;
    pool->size = config->max_pool_size;
    pool->connections = (struct BoltConnection**) BoltMem_allocate(config->max_pool_size*sizeof(BoltConnection*));
    for (int i = 0; i<config->max_pool_size; i++) {
        pool->connections[i] = BoltConnection_create();
    }
    if (config->transport==BOLT_TRANSPORT_ENCRYPTED) {
        pool->ssl_context = create_ssl_ctx(config->trust, address->host, config->log, id);

        // assign ssl_context to all connections
        for (int i = 0; i<config->max_pool_size; i++) {
            pool->connections[0]->ssl_context = pool->ssl_context;
            pool->connections[0]->owns_ssl_context = 0;
        }
    }
    else {
        pool->ssl_context = NULL;
    }
    return pool;
}

void BoltDirectPool_destroy(struct BoltDirectPool* pool)
{
    BoltLog_info(pool->config->log, "[%s]: Destroying pool towards %s:%s", pool->id, pool->address->host,
            pool->address->port);
    for (int index = 0; index<pool->size; index++) {
        close_pool_entry(pool, index);
        BoltConnection_destroy(pool->connections[index]);
    }
    BoltMem_deallocate(pool->connections, pool->size*sizeof(BoltConnection*));
    if (pool->ssl_context!=NULL) {
        free_ssl_context(pool->ssl_context);
    }
    BoltAddress_destroy(pool->address);
    BoltMem_deallocate(pool->id, MAX_ID_LEN);
    BoltSync_cond_destroy(&pool->released_cond);
    BoltSync_mutex_destroy(&pool->mutex);
    BoltMem_deallocate(pool, SIZE_OF_DIRECT_POOL);
}

BoltConnection* BoltDirectPool_acquire(struct BoltDirectPool* pool, BoltStatus* status)
{
    int index = 0;
    int pool_error;
    BoltConnection* connection = NULL;

    BoltLog_info(pool->config->log, "[%s]: Acquiring connection from the pool towards %s:%s", pool->id,
            pool->address->host, pool->address->port);

    BoltSync_mutex_lock(&pool->mutex);

    while (1) {
        index = find_unused_connection(pool);
        pool_error = index>=0 ? BOLT_SUCCESS : BOLT_POOL_FULL;
        if (index>=0) {
            switch (pool->connections[index]->status->state) {
            case BOLT_CONNECTION_STATE_DISCONNECTED:
            case BOLT_CONNECTION_STATE_DEFUNCT:
                // if the connection is DISCONNECTED or DEFUNCT then try
                // to open and initialise it before handing it out.
                pool_error = open_init(pool, index);
                break;
            case BOLT_CONNECTION_STATE_CONNECTED:
                // If CONNECTED, the connection will need to be initialised.
                // This state should rarely, if ever, be encountered here.
                pool_error = init(pool, index);
                break;
            case BOLT_CONNECTION_STATE_FAILED:
                // If FAILED, attempt to RESET the connection, reopening
                // from scratch if that fails. This state should rarely,
                // if ever, be encountered here.
                pool_error = reset_or_open_init(pool, index);
                break;
            case BOLT_CONNECTION_STATE_READY:
                // If the connection is already in the READY state then
                // do nothing and assume that the connection hasn't been
                // timed out by some piece of network housekeeping
                // infrastructure. Such timeouts should instead be managed
                // by setting the maximum connection lifetime.
                break;
            }
        }

        status->state = BOLT_CONNECTION_STATE_DISCONNECTED;
        status->error = BOLT_SUCCESS;
        status->error_ctx = NULL;
        status->error_ctx_size = 0;

        switch (pool_error) {
        case BOLT_SUCCESS:
            connection = pool->connections[index];
            connection->agent = "USED";
            status->state = connection->status->state;
            break;
        case BOLT_CONNECTION_HAS_MORE_INFO:
            status->state = pool->connections[index]->status->state;
            status->error = pool->connections[index]->status->error;
            status->error_ctx = pool->connections[index]->status->error_ctx;
            break;
        default:
            status->state = BOLT_CONNECTION_STATE_DISCONNECTED;
            status->error = pool_error;
            status->error_ctx = NULL;
            break;
        }

        // Retry acquire operation until we get a live connection or timeout
        if (status->error==BOLT_POOL_FULL && pool->config->max_connection_acquisition_time>0) {
            BoltLog_info(pool->config->log,
                    "[%s]: Pool towards %s:%s is full, waiting for a released connection.", pool->id,
                    pool->address->host, pool->address->port);

            if (BoltSync_cond_timedwait(&pool->released_cond, &pool->mutex,
                    pool->config->max_connection_acquisition_time)) {
                continue;
            }

            status->state = BOLT_CONNECTION_STATE_DISCONNECTED;
            status->error = BOLT_POOL_ACQUISITION_TIMED_OUT;
            status->error_ctx = NULL;
        }

        break;
    }

    BoltSync_mutex_unlock(&pool->mutex);

    return connection;
}

int BoltDirectPool_release(struct BoltDirectPool* pool, struct BoltConnection* connection)
{
    BoltLog_info(pool->config->log, "[%s]: Releasing connection to pool towards %s:%s", pool->id, pool->address->host,
            pool->address->port);
    BoltSync_mutex_lock(&pool->mutex);
    int index = find_connection(pool, connection);
    if (index>=0) {
        connection->agent = NULL;

        connection->protocol->clear_run(connection);
        connection->protocol->clear_begin_tx(connection);

        reset_or_close(pool, index);

        BoltSync_cond_signal(&pool->released_cond);
    }
    BoltSync_mutex_unlock(&pool->mutex);
    return index;
}

int BoltDirectPool_connections_in_use(struct BoltDirectPool* pool)
{
    int count = 0;
    for (int i = 0; i<pool->size; i++) {
        if (pool->connections[i]->agent!=NULL) {
            count++;
        }
    }
    return count;
}

