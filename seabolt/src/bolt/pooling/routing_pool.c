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

#include "routing_pool.h"

#include "bolt/connections.h"
#include "bolt/mem.h"
#include "direct_pool.h"

#define SIZE_OF_ADDRESS_P sizeof(struct BoltAddress *)
#define SIZE_OF_ADDRESS_SET sizeof(struct BoltAddressSet)
#define SIZE_OF_ROUTING_TABLE sizeof(struct RoutingTable)
#define SIZE_OF_CONNECTION_POOL_P sizeof(struct BoltConnectionPool *)
#define SIZE_OF_ROUTING_CONNECTION_POOL sizeof(struct BoltRoutingConnectionPool)

struct BoltAddressSet* BoltAddressSet_create()
{
    struct BoltAddressSet* set = (struct BoltAddressSet*) BoltMem_allocate(SIZE_OF_ADDRESS_SET);
    set->size = 0;
    set->elements = NULL;
    return set;
}

void BoltAddressSet_destroy(struct BoltAddressSet* set)
{
    for (int i = 0; i<set->size; i++) {
        BoltAddress_destroy(set->elements[i]);
    }
    BoltMem_deallocate(set->elements, set->size*SIZE_OF_ADDRESS_P);
    BoltMem_deallocate(set, SIZE_OF_ADDRESS_SET);
}

int BoltAddressSet_size(struct BoltAddressSet* set)
{
    return set->size;
}

int BoltAddressSet_indexOf(struct BoltAddressSet* set, struct BoltAddress address)
{
    for (int i = 0; i<set->size; i++) {
        struct BoltAddress* current = set->elements[i];

        if (strcmp(address.host, current->host)==0 && strcmp(address.port, current->port)==0) {
            return i;
        }
    }

    return -1;
}

int BoltAddressSet_add(struct BoltAddressSet* set, struct BoltAddress address)
{
    if (BoltAddressSet_indexOf(set, address)==-1) {
        set->elements = (struct BoltAddress**) BoltMem_reallocate(set->elements, set->size*SIZE_OF_ADDRESS_P,
                (set->size+1)*SIZE_OF_ADDRESS_P);
        set->elements[set->size] = BoltAddress_create(address.host, address.port);
        set->size++;
        return set->size-1;
    }

    return -1;
}

int BoltAddressSet_remove(struct BoltAddressSet* set, struct BoltAddress address)
{
    int index = BoltAddressSet_indexOf(set, address);
    if (index>=0) {
        struct BoltAddress** old_elements = set->elements;
        struct BoltAddress** new_elements = (struct BoltAddress**) BoltMem_allocate((set->size-1)*SIZE_OF_ADDRESS_P);
        int new_elements_index = 0;
        for (int i = 0; i<set->size; i++) {
            if (i==index) {
                continue;
            }

            new_elements[new_elements_index++] = old_elements[i];
        }

        BoltAddress_destroy(set->elements[index]);
        BoltMem_deallocate(old_elements, set->size*SIZE_OF_ADDRESS_P);
        set->elements = new_elements;
        set->size--;
        return index;
    }
    return -1;
}

void BoltAddressSet_replace(struct BoltAddressSet* set, struct BoltAddressSet* others)
{
    for (int i = 0; i<set->size; i++) {
        BoltAddress_destroy(set->elements[i]);
    }

    set->elements = (struct BoltAddress**) BoltMem_reallocate(set->elements, set->size*SIZE_OF_ADDRESS_P,
            others->size*SIZE_OF_ADDRESS_P);
    for (int i = 0; i<others->size; i++) {
        set->elements[i] = BoltAddress_create(others->elements[i]->host, others->elements[i]->port);
    }
    set->size = others->size;
}

void BoltAddressSet_add_all(struct BoltAddressSet* set, struct BoltAddressSet* others)
{
    for (int i = 0; i<others->size; i++) {
        BoltAddressSet_add(set, *others->elements[i]);
    }
}

struct RoutingTable* RoutingTable_create()
{
    struct RoutingTable* table = (struct RoutingTable*) BoltMem_allocate(SIZE_OF_ROUTING_TABLE);
    table->expires = 0;
    table->last_updated = 0;
    table->initial_routers = BoltAddressSet_create();
    table->readers = BoltAddressSet_create();
    table->writers = BoltAddressSet_create();
    table->routers = BoltAddressSet_create();
    return table;
}

struct RoutingTable* RoutingTable_destroy(struct RoutingTable* state)
{
    BoltAddressSet_destroy(state->initial_routers);
    BoltAddressSet_destroy(state->readers);
    BoltAddressSet_destroy(state->writers);
    BoltAddressSet_destroy(state->routers);
    BoltMem_deallocate(state, SIZE_OF_ROUTING_TABLE);
}

int RoutingTable_update(struct RoutingTable* table, struct BoltValue* response)
{
    assert(BoltValue_type(response)==BOLT_DICTIONARY);

    int64_t ttl = 0;
    struct BoltValue* ttl_value = BoltDictionary_value_by_key(response, "ttl", 3);
    if (ttl_value==NULL) {
        return -1;
    }
    assert(BoltValue_type(ttl_value)==BOLT_INTEGER);
    ttl = BoltInteger_get(ttl_value)*1000;

    struct BoltValue* servers_value = BoltDictionary_value_by_key(response, "servers", 7);
    if (servers_value==NULL) {
        return -1;
    }
    assert(BoltValue_type(servers_value)==BOLT_LIST);

    struct BoltAddressSet* readers = BoltAddressSet_create();
    struct BoltAddressSet* writers = BoltAddressSet_create();
    struct BoltAddressSet* routers = BoltAddressSet_create();

    for (int32_t i = 0; i<servers_value->size; i++) {
        struct BoltValue* server_value = BoltList_value(servers_value, i);
        assert(BoltValue_type(server_value)==BOLT_DICTIONARY);

        struct BoltValue* role_value = BoltDictionary_value_by_key(server_value, "role", 4);
        struct BoltValue* addresses_value = BoltDictionary_value_by_key(server_value, "addresses", 9);

        if (role_value==NULL) {
            return -1;
        }
        assert(BoltValue_type(role_value)==BOLT_STRING);

        if (addresses_value==NULL) {
            return -1;
        }
        assert(BoltValue_type(addresses_value)==BOLT_LIST);

        char* role = BoltString_get(role_value);
        for (int32_t j = 0; j<addresses_value->size; j++) {
            struct BoltValue* address_value = BoltList_value(addresses_value, j);

            char* address_str = (char*) BoltMem_allocate(address_value->size+1);
            strncpy(address_str, BoltString_get(address_value), address_value->size);
            address_str[address_value->size] = '\0';

            char* port_str = strrchr(address_str, ':');
            if (port_str==NULL) {
                return -1;
            }
            *port_str++ = '\0';

            struct BoltAddress address = BoltAddress_of(address_str, port_str);
            if (strcmp(role, "READ")==0) {
                BoltAddressSet_add(readers, address);
            }
            else if (strcmp(role, "WRITE")==0) {
                BoltAddressSet_add(writers, address);
            }
            else if (strcmp(role, "ROUTE")==0) {
                BoltAddressSet_add(routers, address);
            }
            else {
                return -1;
            }
            BoltMem_deallocate(address_str, address_value->size+1);
        }
    }

    BoltAddressSet_replace(table->readers, readers);
    BoltAddressSet_replace(table->writers, writers);
    BoltAddressSet_replace(table->routers, routers);
    table->last_updated = BoltUtil_get_time_ms();
    table->expires = table->last_updated+ttl;

    BoltAddressSet_destroy(readers);
    BoltAddressSet_destroy(writers);
    BoltAddressSet_destroy(routers);

    return BOLT_SUCCESS;
}

int RoutingTable_is_expired(struct RoutingTable* state, enum BoltAccessMode mode)
{
    return state->routers->size==0
            || (mode==BOLT_ACCESS_MODE_READ ? state->readers->size==0 : state->writers->size==0)
            || state->expires<=BoltUtil_get_time_ms();
}

int _routing_pool_ensure_server(struct BoltRoutingConnectionPool* pool, struct BoltAddress* server)
{
    int index = -1;
    for (int i = 0; i<pool->size; i++) {
        struct BoltAddress* pool_server = pool->servers[i];

        if (strcmp(server->host, pool_server->host)==0 && strcmp(server->port, pool_server->port)==0) {
            index = i;
            break;
        }
    }

    if (index<0) {
        BoltUtil_mutex_lock(&pool->lock);
        index = pool->size;
        pool->size++;

        pool->servers = (struct BoltAddress**) BoltMem_reallocate(pool->servers, (pool->size-1)*SIZE_OF_ADDRESS_P,
                pool->size*SIZE_OF_ADDRESS_P);
        pool->servers[index] = BoltAddress_create(server->host, server->port);

        pool->server_pools = (struct BoltConnectionPool**) BoltMem_reallocate(pool->server_pools,
                (pool->size-1)*SIZE_OF_CONNECTION_POOL_P,
                pool->size*SIZE_OF_CONNECTION_POOL_P);
        pool->server_pools[index] = BoltConnectionPool_create(server, pool->config);
        BoltUtil_mutex_unlock(&pool->lock);
    }

    return index;
}

int _routing_table_fetch(struct BoltRoutingConnectionPool* pool, struct BoltAddress* server)
{
    int index = _routing_pool_ensure_server(pool, server);
    struct BoltConnectionResult result = BoltConnectionPool_acquire(pool->server_pools[index]);
    if (result.connection!=NULL) {
        const char* routing_table_call = "CALL dbms.cluster.routing.getRoutingTable($context)";

        int status = BoltConnection_cypher(result.connection, routing_table_call, strlen(routing_table_call), 1);
        if (status!=BOLT_SUCCESS) {
            BoltConnectionPool_release(pool->server_pools[index], result.connection);
            return status;
        }

        struct BoltValue* routing_table_context = BoltConnection_cypher_parameter(result.connection, 0, "context", 7);
        if (pool->config->routing_context!=NULL) {
            BoltValue_copy(routing_table_context, pool->config->routing_context);
        }

        status = BoltConnection_load_run_request(result.connection);
        if (status!=BOLT_SUCCESS) {
            BoltConnectionPool_release(pool->server_pools[index], result.connection);
            return status;
        }

        status = BoltConnection_load_pull_request(result.connection, -1);
        if (status!=BOLT_SUCCESS) {
            BoltConnectionPool_release(pool->server_pools[index], result.connection);
            return status;
        }

        int pull_all = BoltConnection_last_request(result.connection);

        status = BoltConnection_send(result.connection);
        if (status!=BOLT_SUCCESS) {
            BoltConnectionPool_release(pool->server_pools[index], result.connection);
            return status;
        }

        struct BoltValue* response = NULL;
        while (BoltConnection_fetch(result.connection, pull_all)>0) {
            if (response!=NULL) {
                BoltConnectionPool_release(pool->server_pools[index], result.connection);
                // TODO: it is not expected to receive records > 1
                return -1;
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

        if (response==NULL) {
            BoltConnectionPool_release(pool->server_pools[index], result.connection);
            return -1;
        }

        status = RoutingTable_update(pool->routing_table, response);
        if (status!=BOLT_SUCCESS) {
            BoltValue_destroy(response);

            return status;
        }

        BoltValue_destroy(response);

        return BOLT_SUCCESS;
    }
    return result.connection_error;
}

int _routing_table_refresh(struct BoltRoutingConnectionPool* pool)
{
    int result = BOLT_ROUTING_UNABLE_TO_RETRIEVE_ROUTING_TABLE;

    struct BoltAddressSet* routers = BoltAddressSet_create();
    BoltAddressSet_add_all(routers, pool->routing_table->routers);
    BoltAddressSet_add_all(routers, pool->routing_table->initial_routers);

    for (int i = 0; i<routers->size; i++) {
        int status = _routing_table_fetch(pool, routers->elements[i]);
        if (status==BOLT_SUCCESS) {
            result = BOLT_SUCCESS;
            break;
        }
    }

    BoltAddressSet_destroy(routers);

    return result;
}

int _routing_table_ensure_fresh(struct BoltRoutingConnectionPool* pool, enum BoltAccessMode mode)
{
    int status = BOLT_SUCCESS;

    if (RoutingTable_is_expired(pool->routing_table, mode)) {
        BoltUtil_mutex_lock(&pool->lock);
        if (RoutingTable_is_expired(pool->routing_table, mode)) {
            status = _routing_table_refresh(pool);
        }
        BoltUtil_mutex_unlock(&pool->lock);
    }

    return status;
}

struct BoltAddress* _select_server(struct BoltRoutingConnectionPool* pool, struct BoltAddressSet* servers, int offset)
{
    int start_index = offset%servers->size;
    int index = start_index;
    struct BoltAddress* least_connected_server = NULL;
    int least_connected = INT_MAX;
    while (1) {
        struct BoltAddress* server = servers->elements[index];
        int pool_index = _routing_pool_ensure_server(pool, server);
        struct BoltConnectionPool* server_pool = pool->server_pools[pool_index];

        int server_active_connections = BoltConnectionPool_connections_in_use(server_pool);
        if (server_active_connections<least_connected) {
            least_connected_server = server;
            least_connected = server_active_connections;
        }

        index = (index+1)%servers->size;
        if (index==start_index) {
            break;
        }
    }

    return least_connected_server;
}

struct BoltAddress* _select_reader(struct BoltRoutingConnectionPool* pool)
{
    struct BoltAddress* server = _select_server(pool, pool->routing_table->readers, pool->readers_offset);
    pool->readers_offset++;
    return server;
}

struct BoltAddress* _select_writer(struct BoltRoutingConnectionPool* pool)
{
    struct BoltAddress* server = _select_server(pool, pool->routing_table->writers, pool->writers_offset);
    pool->writers_offset++;
    return server;
}

struct BoltRoutingConnectionPool*
BoltRoutingConnectionPool_create(struct BoltAddress* address, struct BoltConfig* config)
{
    struct BoltRoutingConnectionPool* pool = (struct BoltRoutingConnectionPool*) BoltMem_allocate(
            SIZE_OF_ROUTING_CONNECTION_POOL);

    pool->config = config;

    pool->size = 0;
    pool->servers = NULL;
    pool->server_pools = NULL;

    pool->routing_table = RoutingTable_create();
    // TODO: custom resolver?
    BoltAddressSet_add(pool->routing_table->initial_routers, BoltAddress_of(address->host, address->port));
    pool->readers_offset = 0;
    pool->writers_offset = 0;

    BoltUtil_mutex_create(&pool->lock);

    return pool;
}

void BoltRoutingConnectionPool_destroy(struct BoltRoutingConnectionPool* pool)
{
    for (int i = 0; i<pool->size; i++) {
        BoltAddress_destroy(pool->servers[i]);
        BoltConnectionPool_destroy(pool->server_pools[i]);
    }
    BoltMem_deallocate(pool->server_pools, pool->size*SIZE_OF_CONNECTION_POOL_P);
    BoltMem_deallocate(pool->servers, pool->size*SIZE_OF_ADDRESS_P);

    RoutingTable_destroy(pool->routing_table);

    BoltUtil_mutex_destroy(&pool->lock);

    BoltMem_deallocate(pool, SIZE_OF_ROUTING_CONNECTION_POOL);
}

struct BoltConnectionResult
BoltRoutingConnectionPool_acquire(struct BoltRoutingConnectionPool* pool, enum BoltAccessMode mode)
{
    int status = _routing_table_ensure_fresh(pool, mode);
    if (status==BOLT_SUCCESS) {
        struct BoltAddress* server = mode==BOLT_ACCESS_MODE_READ ? _select_reader(pool) : _select_writer(pool);
        if (server==NULL) {
            return (struct BoltConnectionResult) {
                    NULL, BOLT_DISCONNECTED, BOLT_ROUTING_NO_SERVERS_TO_SELECT, NULL
            };
        }

        int server_pool_index = _routing_pool_ensure_server(pool, server);
        if (server_pool_index<0) {
            return (struct BoltConnectionResult) {
                    NULL, BOLT_DISCONNECTED, BOLT_ROUTING_UNABLE_TO_CONSTRUCT_POOL_FOR_SERVER, NULL
            };
        }

        struct BoltConnectionResult inner_result = BoltConnectionPool_acquire(pool->server_pools[server_pool_index]);
        return inner_result;
    }

    return (struct BoltConnectionResult) {
            NULL, BOLT_DISCONNECTED, BOLT_ROUTING_UNABLE_TO_REFRESH_ROUTING_TABLE, NULL
    };
}

int BoltRoutingConnectionPool_release(struct BoltRoutingConnectionPool* pool, struct BoltConnection* connection)
{
    struct BoltAddress* server = BoltAddress_create(connection->host, connection->port);

    int server_pool_index = _routing_pool_ensure_server(pool, server);
    BoltAddress_destroy(server);
    if (server_pool_index<0) {
        BoltConnection_close(connection);
    }

    BoltConnectionPool_release(pool->server_pools[server_pool_index], connection);
}
