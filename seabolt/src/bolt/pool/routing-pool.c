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
#include <string.h>
#include <assert.h>

#include "bolt/config-impl.h"
#include "routing-pool.h"

#include "bolt/connections.h"
#include "bolt/mem.h"

#include "routing-table.h"
#include "direct-pool.h"
#include "bolt/address-set.h"

int BoltRoutingPool_ensure_server(struct BoltRoutingPool* pool, const struct BoltAddress* server)
{
    int index = BoltAddressSet_index_of(pool->servers, *server);

    if (index<0) {
        BoltUtil_mutex_lock(&pool->lock);

        // Add and return the index (which is always the end of the set - there's no ordering in place)
        index = BoltAddressSet_add(pool->servers, *server);

        // Expand the direct pools and create a new one for this server
        pool->server_pools = (struct BoltDirectPool**) BoltMem_reallocate(pool->server_pools,
                (pool->servers->size-1)*
                        SIZE_OF_DIRECT_POOL_PTR,
                (pool->servers->size)*
                        SIZE_OF_DIRECT_POOL_PTR);
        pool->server_pools[index] = BoltDirectPool_create(server, pool->auth_token, pool->config);

        BoltUtil_mutex_unlock(&pool->lock);
    }

    return index;
}

int BoltRoutingPool_update_routing_table_from(struct BoltRoutingPool* pool, struct BoltAddress* server)
{
    int index = BoltRoutingPool_ensure_server(pool, server);
    struct BoltConnectionResult result = BoltDirectPool_acquire(pool->server_pools[index]);
    if (result.connection==NULL) {
        return result.connection_error;
    }

    // TODO: support for 3.2 (different discovery procedure)?
    int status = BOLT_SUCCESS;
    const char* routing_table_call = "CALL dbms.cluster.routing.getRoutingTable($context)";

    // Load Run message filled with discovery procedure along with routing context
    if (status==BOLT_SUCCESS) {
        status = BoltConnection_cypher(result.connection, routing_table_call, strlen(routing_table_call), 1);

        if (status==BOLT_SUCCESS) {
            struct BoltValue* routing_table_context = BoltConnection_cypher_parameter(result.connection, 0, "context",
                    7);
            if (pool->config->routing_context!=NULL) {
                BoltValue_copy(routing_table_context, pool->config->routing_context);
            }

            status = BoltConnection_load_run_request(result.connection);
        }
    }

    // Load Pull All message
    if (status==BOLT_SUCCESS) {
        status = BoltConnection_load_pull_request(result.connection, -1);
    }

    // Send pending messages
    bolt_request_t pull_all = 0;
    if (status==BOLT_SUCCESS) {
        pull_all = BoltConnection_last_request(result.connection);

        status = BoltConnection_send(result.connection);
    }

    // Retrieve results
    struct BoltValue* response = NULL;
    if (status==BOLT_SUCCESS) {
        while (BoltConnection_fetch(result.connection, pull_all)>0) {
            // We don't expect more than 1 record being returned
            if (response!=NULL) {
                status = BOLT_ROUTING_UNEXPECTED_DISCOVERY_RESPONSE;
                break;
            }

            struct BoltValue* field_keys = BoltConnection_fields(result.connection);
            struct BoltValue* field_values = BoltConnection_record_fields(result.connection);

            response = BoltValue_create();
            BoltValue_format_as_Dictionary(response, field_keys->size);
            for (int i = 0; i<field_keys->size; i++) {
                BoltValue_copy(BoltDictionary_key(response, i), BoltList_value(field_keys, i));
                BoltValue_copy(BoltDictionary_value(response, i), BoltList_value(field_values, i));
            }
        }

        // We didn't receive any records
        if (response==NULL) {
            status = BOLT_ROUTING_UNEXPECTED_DISCOVERY_RESPONSE;
        }
    }

    if (status==BOLT_SUCCESS) {
        status = RoutingTable_update(pool->routing_table, response);
    }

    BoltValue_destroy(response);
    BoltDirectPool_release(pool->server_pools[index], result.connection);

    return status;
}

enum BoltConnectionError BoltRoutingPool_update_routing_table(struct BoltRoutingPool* pool)
{
    enum BoltConnectionError result = BOLT_ROUTING_UNABLE_TO_RETRIEVE_ROUTING_TABLE;

    // discover initial routers which pass through address resolver
    struct BoltAddressSet* initial_routers = BoltAddressSet_create();
    // first try to resolve using address resolver callback if specified
    BoltAddressResolver_resolve(pool->config->address_resolver, pool->address, initial_routers);
    // if nothing got added to the initial router addresses, add the connector hostname and port
    if (initial_routers->size==0) {
        BoltAddressSet_add(initial_routers, *pool->address);
    }

    // Create a set of servers to update the routing table from
    struct BoltAddressSet* routers = BoltAddressSet_create();

    // First add routers present in the routing table
    BoltAddressSet_add_all(routers, pool->routing_table->routers);
    // Then add initial routers
    BoltAddressSet_add_all(routers, initial_routers);

    // Try each in turn until successful
    for (int i = 0; i<routers->size; i++) {
        BoltLog_debug(pool->config->log, "trying routing table update from server '%s:%s'", routers->elements[i]->host,
                routers->elements[i]->port);

        int status = BoltRoutingPool_update_routing_table_from(pool, routers->elements[i]);
        if (status==BOLT_SUCCESS) {
            result = BOLT_SUCCESS;
            break;
        }
    }

    // Destroy the set of servers
    BoltAddressSet_destroy(routers);
    BoltAddressSet_destroy(initial_routers);

    return result;
}

void BoltRoutingPool_cleanup(struct BoltRoutingPool* pool)
{
    struct BoltAddressSet* active_servers = BoltAddressSet_create();
    BoltAddressSet_add_all(active_servers, pool->routing_table->routers);
    BoltAddressSet_add_all(active_servers, pool->routing_table->writers);
    BoltAddressSet_add_all(active_servers, pool->routing_table->readers);

    struct BoltAddressSet* old_servers = pool->servers;
    struct BoltDirectPool** old_server_pools = pool->server_pools;
    int old_size = old_servers->size;

    int cleanup_count = 0;
    int* cleanup_marker = (int*) BoltMem_allocate(old_size*sizeof(int));
    for (int i = 0; i<old_size; i++) {
        cleanup_marker[i] = BoltAddressSet_index_of(active_servers, *old_servers->elements[i])<0
                && BoltDirectPool_connections_in_use(old_server_pools[i])==0;
        cleanup_count = cleanup_count+cleanup_marker[i];
    }

    if (cleanup_count>0) {
        int new_size = old_size-cleanup_count;

        struct BoltAddressSet* new_servers = BoltAddressSet_create();
        struct BoltDirectPool** new_server_pools = (struct BoltDirectPool**) BoltMem_allocate(
                new_size*SIZE_OF_DIRECT_POOL_PTR);

        for (int i = 0; i<old_size; i++) {
            if (cleanup_marker[i]) {
                continue;
            }

            int index = BoltAddressSet_add(new_servers, *old_servers->elements[i]);
            new_server_pools[index] = old_server_pools[i];
        }

        pool->servers = new_servers;
        pool->server_pools = new_server_pools;

        for (int i = 0; i<old_size; i++) {
            if (cleanup_marker[i]) {
                BoltDirectPool_destroy(old_server_pools[i]);
            }
        }

        BoltAddressSet_destroy(old_servers);
        BoltMem_deallocate(old_server_pools, old_size*SIZE_OF_DIRECT_POOL_PTR);
    }

    BoltMem_deallocate(cleanup_marker, old_size*sizeof(int));
    BoltAddressSet_destroy(active_servers);
}

enum BoltConnectionError BoltRoutingPool_ensure_routing_table(struct BoltRoutingPool* pool, enum BoltAccessMode mode)
{
    enum BoltConnectionError status = BOLT_SUCCESS;

    // Is routing table refresh wrt the requested access mode?
    if (RoutingTable_is_expired(pool->routing_table, mode)) {
        // Lock
        BoltUtil_mutex_lock(&pool->lock);

        // Is routing table still refresh wrt the requested access mode? (i.e. another thread could have updated it
        // while we were waiting for acquiring the lock
        if (RoutingTable_is_expired(pool->routing_table, mode)) {
            BoltLog_debug(pool->config->log, "routing table is expired, starting refresh");

            status = BoltRoutingPool_update_routing_table(pool);
            if (status==BOLT_SUCCESS) {
                BoltLog_debug(pool->config->log, "routing table is updated, calling cleanup on server pools");

                BoltRoutingPool_cleanup(pool);

                BoltLog_debug(pool->config->log, "server pools cleanup completed");
            }
            else {
                BoltLog_debug(pool->config->log, "routing table update failed with code %d", status);
            }
        }

        // Unlock
        BoltUtil_mutex_unlock(&pool->lock);
    }

    return status;
}

struct BoltAddress* BoltRoutingPool_select_least_connected(struct BoltRoutingPool* pool,
        struct BoltAddressSet* servers, int offset)
{
    // Start at an index that round-robins
    int start_index = offset%servers->size;
    int index = start_index;

    struct BoltAddress* least_connected_server = NULL;
    int least_connected = INT_MAX;

    while (1) {
        // Pick the current server
        struct BoltAddress* server = servers->elements[index];

        // Retrieve related direct pool
        int pool_index = BoltRoutingPool_ensure_server(pool, server);
        struct BoltDirectPool* server_pool = pool->server_pools[pool_index];

        // Compare in use connections to what we currently have and update if this is less
        int server_active_connections = BoltDirectPool_connections_in_use(server_pool);
        if (server_active_connections<least_connected) {
            least_connected_server = server;
            least_connected = server_active_connections;
        }

        // Stop if we had cycled through all servers
        index = (index+1)%servers->size;
        if (index==start_index) {
            break;
        }
    }

    return least_connected_server;
}

struct BoltAddress* BoltRoutingPool_select_least_connected_reader(struct BoltRoutingPool* pool)
{
    struct BoltAddress* server = BoltRoutingPool_select_least_connected(pool, pool->routing_table->readers,
            pool->readers_offset);
    pool->readers_offset++;
    return server;
}

struct BoltAddress* BoltRoutingPool_select_least_connected_writer(struct BoltRoutingPool* pool)
{
    struct BoltAddress* server = BoltRoutingPool_select_least_connected(pool, pool->routing_table->writers,
            pool->writers_offset);
    pool->writers_offset++;
    return server;
}

void BoltRoutingPool_forget_server(struct BoltRoutingPool* pool, const struct BoltAddress* server)
{
    // Lock
    BoltUtil_mutex_lock(&pool->lock);

    RoutingTable_forget_server(pool->routing_table, *server);
    BoltRoutingPool_cleanup(pool);

    // Unlock
    BoltUtil_mutex_unlock(&pool->lock);
}

void BoltRoutingPool_forget_writer(struct BoltRoutingPool* pool, const struct BoltAddress* server)
{
    // Lock
    BoltUtil_mutex_lock(&pool->lock);

    RoutingTable_forget_writer(pool->routing_table, *server);
    BoltRoutingPool_cleanup(pool);

    // Unlock
    BoltUtil_mutex_unlock(&pool->lock);
}

void BoltRoutingPool_handle_connection_error_by_code(struct BoltRoutingPool* pool, const struct BoltAddress* server,
        enum BoltConnectionError code)
{
    switch (code) {
    case BOLT_ROUTING_UNABLE_TO_RETRIEVE_ROUTING_TABLE:
    case BOLT_ROUTING_NO_SERVERS_TO_SELECT:
    case BOLT_ROUTING_UNABLE_TO_CONSTRUCT_POOL_FOR_SERVER:
    case BOLT_ROUTING_UNABLE_TO_REFRESH_ROUTING_TABLE:
    case BOLT_ROUTING_UNEXPECTED_DISCOVERY_RESPONSE:
        BoltRoutingPool_forget_server(pool, server);
        break;
    case BOLT_INTERRUPTED:
    case BOLT_CONNECTION_RESET:
    case BOLT_NO_VALID_ADDRESS:
    case BOLT_TIMED_OUT:
    case BOLT_CONNECTION_REFUSED:
    case BOLT_NETWORK_UNREACHABLE:
    case BOLT_TLS_ERROR:
    case BOLT_END_OF_TRANSMISSION:
        BoltRoutingPool_forget_server(pool, server);
        break;
    case BOLT_ADDRESS_NOT_RESOLVED:
        BoltRoutingPool_forget_server(pool, server);
        break;
    default:
        break;
    }
}

void BoltRoutingPool_handle_connection_error_by_failure(struct BoltRoutingPool* pool, const struct BoltAddress* server,
        const struct BoltValue* failure)
{
    if (failure==NULL) {
        return;
    }

    struct BoltValue* code = BoltDictionary_value_by_key(failure, "code", 4);
    if (code==NULL) {
        return;
    }

    char* code_str = (char*) BoltMem_allocate((code->size+1)*sizeof(char));
    strncpy(code_str, BoltString_get(code), code->size);
    code_str[code->size] = '\0';

    if (strcmp(code_str, "Neo.ClientError.General.ForbiddenOnReadOnlyDatabase")==0) {
        BoltRoutingPool_forget_writer(pool, server);
    }
    else if (strcmp(code_str, "Neo.ClientError.Cluster.NotALeader")==0) {
        BoltRoutingPool_forget_writer(pool, server);
    }
    else if (strcmp(code_str, "Neo.TransientError.General.DatabaseUnavailable")==0) {
        BoltRoutingPool_forget_server(pool, server);
    }

    BoltMem_deallocate(code_str, (code->size+1)*sizeof(char));
}

void BoltRoutingPool_handle_connection_error(struct BoltRoutingPool* pool, struct BoltConnection* connection)
{
    switch (connection->error) {
    case BOLT_SUCCESS:
        break;
    case BOLT_SERVER_FAILURE:
        BoltRoutingPool_handle_connection_error_by_failure(pool, connection->address,
                BoltConnection_failure(connection));
        break;
    default:
        BoltRoutingPool_handle_connection_error_by_code(pool, connection->address, connection->error);
        break;
    }
}

void BoltRoutingPool_connection_error_handler(struct BoltConnection* connection, void* state)
{
    BoltRoutingPool_handle_connection_error((struct BoltRoutingPool*) state, connection);
}

struct BoltRoutingPool*
BoltRoutingPool_create(struct BoltAddress* address, const struct BoltValue* auth_token, const struct BoltConfig* config)
{
    struct BoltRoutingPool* pool = (struct BoltRoutingPool*) BoltMem_allocate(SIZE_OF_ROUTING_POOL);

    pool->address = address;
    pool->config = config;
    pool->auth_token = auth_token;

    pool->servers = BoltAddressSet_create();
    pool->server_pools = NULL;

    pool->routing_table = RoutingTable_create();
    pool->readers_offset = 0;
    pool->writers_offset = 0;

    BoltUtil_mutex_create(&pool->lock);

    return pool;
}

void BoltRoutingPool_destroy(struct BoltRoutingPool* pool)
{
    for (int i = 0; i<pool->servers->size; i++) {
        BoltDirectPool_destroy(pool->server_pools[i]);
    }
    BoltMem_deallocate(pool->server_pools, pool->servers->size*SIZE_OF_DIRECT_POOL_PTR);
    BoltAddressSet_destroy(pool->servers);

    RoutingTable_destroy(pool->routing_table);

    BoltUtil_mutex_destroy(&pool->lock);

    BoltMem_deallocate(pool, SIZE_OF_ROUTING_POOL);
}

struct BoltConnectionResult
BoltRoutingPool_acquire(struct BoltRoutingPool* pool, enum BoltAccessMode mode)
{
    struct BoltAddress* server = NULL;

    enum BoltConnectionError status = BoltRoutingPool_ensure_routing_table(pool, mode);
    if (status==BOLT_SUCCESS) {
        server = mode==BOLT_ACCESS_MODE_READ
                 ? BoltRoutingPool_select_least_connected_reader(pool)
                 : BoltRoutingPool_select_least_connected_writer(pool);
        if (server==NULL) {
            status = BOLT_ROUTING_NO_SERVERS_TO_SELECT;
        }
    }

    int server_pool_index = -1;
    if (status==BOLT_SUCCESS) {
        server_pool_index = BoltRoutingPool_ensure_server(pool, server);
        if (server_pool_index<0) {
            status = BOLT_ROUTING_UNABLE_TO_CONSTRUCT_POOL_FOR_SERVER;
        }
    }

    struct BoltConnectionResult result;
    if (status==BOLT_SUCCESS) {
        result = BoltDirectPool_acquire(pool->server_pools[server_pool_index]);
        if (result.connection!=NULL) {
            status = BOLT_SUCCESS;
            result.connection->on_error_cb_state = pool;
            result.connection->on_error_cb = BoltRoutingPool_connection_error_handler;
        }
        else {
            status = result.connection_error;
        }
    }

    if (status==BOLT_SUCCESS) {
        return result;
    }

    // check if we were able to complete server selection. this is not the case when routing
    // table refresh fails.
    if (server!=NULL) {
        BoltRoutingPool_handle_connection_error_by_code(pool, server, status);
    }

    return CONNECTION_RESULT_ERROR(status, NULL);
}

int BoltRoutingPool_release(struct BoltRoutingPool* pool, struct BoltConnection* connection)
{
    connection->on_error_cb = NULL;
    connection->on_error_cb_state = NULL;

    int server_pool_index = BoltRoutingPool_ensure_server(pool, connection->address);
    if (server_pool_index<0) {
        BoltConnection_close(connection);
        return -1;
    }

    return BoltDirectPool_release(pool->server_pools[server_pool_index], connection);
}
