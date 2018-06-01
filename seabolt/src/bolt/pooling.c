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

#include "bolt/config-impl.h"
#include "bolt/logging.h"
#include "bolt/mem.h"
#include "bolt/pooling.h"

#define SIZE_OF_CONNECTION_POOL sizeof(struct BoltConnectionPool)


// TODO: put this somewhere else
void timespec_diff(struct timespec* t, struct timespec* t0, struct timespec* t1)
{
    t->tv_sec = t0->tv_sec - t1->tv_sec;
    t->tv_nsec = t0->tv_nsec - t1->tv_nsec;
    while (t->tv_nsec >= 1000000000)
    {
        t->tv_sec += 1;
        t->tv_nsec -= 1000000000;
    }
    while (t->tv_nsec < 0)
    {
        t->tv_sec -= 1;
        t->tv_nsec += 1000000000;
    }
}


int find_unused_connection(struct BoltConnectionPool * pool)
{
    for (size_t i = 0; i < pool->size; i++)
    {
        struct BoltConnection * connection = &pool->connections[i];
        if (connection->agent == NULL)
        {
            return (int)i;
        }
    }
    return -1;
}

int find_connection(struct BoltConnectionPool * pool, struct BoltConnection * connection)
{
    for (size_t i = 0; i < pool->size; i++)
    {
        struct BoltConnection * candidate = &pool->connections[i];
        if (candidate == connection)
        {
            return (int)i;
        }
    }
    return -1;
}

int init(struct BoltConnectionPool * pool, int index)
{
    struct BoltConnection * connection = &pool->connections[index];
    switch (BoltConnection_init(connection, pool->profile))
    {
        case 0:
            return index;
        default:
            return -1;
    }
}

int open_init(struct BoltConnectionPool * pool, int index)
{
    // Host name resolution is carried out every time a connection
    // is opened. Given that connections are pooled and reused,
    // this is not a huge overhead.
    switch (BoltAddress_resolve(pool->address))
    {
        case 0:
            break;
        default:
            return -1;  // Could not resolve address
    }
    struct BoltConnection * connection = &pool->connections[index];
    switch (BoltConnection_open(connection, pool->transport, pool->address))
    {
        case 0:
            return init(pool, index);
        default:
            return -1;  // Could not open socket
    }
}

void close_pool_entry(struct BoltConnectionPool * pool, int index)
{
    struct BoltConnection * connection = &pool->connections[index];
    if (connection->status != BOLT_DISCONNECTED)
    {
        struct timespec now;
        struct timespec diff;
        BoltUtil_get_time(&now);
        timespec_diff(&diff, &now, &connection->metrics.time_opened);
        BoltLog_info("bolt: Connection alive for %lds %09ldns", (long)(diff.tv_sec), diff.tv_nsec);
        BoltConnection_close(connection);
    }
}

int reset_or_open_init(struct BoltConnectionPool * pool, int index)
{
    struct BoltConnection * connection = &pool->connections[index];
    switch (BoltConnection_reset(connection))
    {
        case 0:
            return index;
        default:
            return open_init(pool, index);
    }
}

void reset_or_close(struct BoltConnectionPool * pool, int index)
{
    struct BoltConnection * connection = &pool->connections[index];
    // TODO: disconnect if too old
    switch (BoltConnection_reset(connection))
    {
        case 0:
            break;
        default:
            close_pool_entry(pool, index);
    }
}

struct BoltUserProfile * copy_profile(const struct BoltUserProfile * profile)
{
	if (profile == NULL)
	{
		return NULL;
	}

	struct BoltUserProfile * new_profile = BoltMem_allocate(sizeof(struct BoltUserProfile));
	new_profile->auth_scheme = profile->auth_scheme;
	new_profile->user = BoltMem_duplicate(profile->user, strlen(profile->user) + 1);
	new_profile->password = BoltMem_duplicate(profile->password, strlen(profile->password) + 1);
	new_profile->user_agent = BoltMem_duplicate(profile->user_agent, strlen(profile->user_agent) + 1);
	return new_profile;
}

void free_profile(struct BoltUserProfile * profile)
{
	if (profile != NULL)
	{
		BoltMem_deallocate(profile->user, strlen(profile->user) + 1);
		BoltMem_deallocate(profile->password, strlen(profile->password) + 1);
		BoltMem_deallocate(profile->user_agent, strlen(profile->user_agent) + 1);
		BoltMem_deallocate((void *)profile, sizeof(struct BoltUserProfile));
	}
}

struct BoltConnectionPool * BoltConnectionPool_create(enum BoltTransport transport, struct BoltAddress * address,
                                                      const struct BoltUserProfile * profile, size_t size)
{
    struct BoltConnectionPool * pool = BoltMem_allocate(SIZE_OF_CONNECTION_POOL);
    BoltUtil_mutex_create(&pool->mutex);
    pool->transport = transport;
    pool->address = address;
	pool->profile = copy_profile(profile);
    pool->size = size;
    pool->connections = BoltMem_allocate(size * sizeof(struct BoltConnection));
    memset(pool->connections, 0, size * sizeof(struct BoltConnection));
    return pool;
}

void BoltConnectionPool_destroy(struct BoltConnectionPool * pool)
{
    for (size_t index = 0; index < pool->size; index++)
    {
        close_pool_entry(pool, (int)index);
    }
    pool->connections = BoltMem_deallocate(pool->connections, pool->size * sizeof(struct BoltConnection));
    BoltUtil_mutex_destroy(&pool->mutex);
    free_profile(pool->profile);
    BoltMem_deallocate(pool, SIZE_OF_CONNECTION_POOL);
}

struct BoltConnection * BoltConnectionPool_acquire(struct BoltConnectionPool * pool, const void * agent)
{
    BoltUtil_mutex_lock(&pool->mutex);
    int index = find_unused_connection(pool);
    if (index >= 0)
    {
        switch (pool->connections[index].status)
        {
            case BOLT_DISCONNECTED:
            case BOLT_DEFUNCT:
                // if the connection is DISCONNECTED or DEFUNCT then try
                // to open and initialise it before handing it out.
                index = open_init(pool, index);
                break;
            case BOLT_CONNECTED:
                // If CONNECTED, the connection will need to be initialised.
                // This state should rarely, if ever, be encountered here.
                // TODO: disconnect if too old
                index = init(pool, index);
                break;
            case BOLT_FAILED:
                // If FAILED, attempt to RESET the connection, reopening
                // from scratch if that fails. This state should rarely,
                // if ever, be encountered here.
                // TODO: disconnect if too old
                index = reset_or_open_init(pool, index);
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
    struct BoltConnection * connection;
    if (index >= 0)
    {
        connection = &pool->connections[index];
        connection->agent = agent;
    }
    else
    {
        connection = NULL;
    }
    BoltUtil_mutex_unlock(&pool->mutex);
    return connection;
}

int BoltConnectionPool_release(struct BoltConnectionPool * pool, struct BoltConnection * connection)
{
    BoltUtil_mutex_lock(&pool->mutex);
    int index = find_connection(pool, connection);
    if (index >= 0)
    {
        connection->agent = NULL;
        reset_or_close(pool, index);
    }
	BoltUtil_mutex_unlock(&pool->mutex);
    return index;
}
