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

#include <assert.h>
#include <string.h>

#include "config-impl.h"
#include "address-set.h"
#include "mem.h"
#include "routing-table.h"

#define READ_ROLE "READ"
#define WRITE_ROLE "WRITE"
#define ROUTE_ROLE "ROUTE"

#define TTL_KEY "ttl"
#define TTL_KEY_LEN 3
#define SERVERS_KEY "servers"
#define SERVERS_KEY_LEN 7
#define ROLE_KEY "role"
#define ROLE_KEY_LEN 4
#define ADDRESSES_KEY "addresses"
#define ADDRESSES_KEY_LEN 9

struct RoutingTable* RoutingTable_create()
{
    struct RoutingTable* table = (struct RoutingTable*) BoltMem_allocate(SIZE_OF_ROUTING_TABLE);
    table->expires = 0;
    table->last_updated = 0;
    table->readers = BoltAddressSet_create();
    table->writers = BoltAddressSet_create();
    table->routers = BoltAddressSet_create();
    return table;
}

void RoutingTable_destroy(struct RoutingTable* state)
{
    BoltAddressSet_destroy(state->readers);
    BoltAddressSet_destroy(state->writers);
    BoltAddressSet_destroy(state->routers);
    BoltMem_deallocate(state, SIZE_OF_ROUTING_TABLE);
}

int RoutingTable_update(struct RoutingTable* table, struct BoltValue* response)
{
    int status = BOLT_SUCCESS;
    if (BoltValue_type(response)!=BOLT_DICTIONARY) {
        status = BOLT_ROUTING_UNEXPECTED_DISCOVERY_RESPONSE;
    }

    int64_t ttl = 0;
    if (status==BOLT_SUCCESS) {
        struct BoltValue* ttl_value = BoltDictionary_value_by_key(response, TTL_KEY, TTL_KEY_LEN);
        if (ttl_value==NULL || BoltValue_type(ttl_value)!=BOLT_INTEGER) {
            status = BOLT_ROUTING_UNEXPECTED_DISCOVERY_RESPONSE;
        }
        ttl = BoltInteger_get(ttl_value)*1000;
    }

    struct BoltValue* servers_value = NULL;
    if (status==BOLT_SUCCESS) {
        servers_value = BoltDictionary_value_by_key(response, SERVERS_KEY, SERVERS_KEY_LEN);
        if (servers_value==NULL || BoltValue_type(servers_value)!=BOLT_LIST) {
            status = BOLT_ROUTING_UNEXPECTED_DISCOVERY_RESPONSE;
        }
    }

    struct BoltAddressSet* readers = BoltAddressSet_create();
    struct BoltAddressSet* writers = BoltAddressSet_create();
    struct BoltAddressSet* routers = BoltAddressSet_create();

    for (int32_t i = 0; i<servers_value->size && status==BOLT_SUCCESS; i++) {
        struct BoltValue* server_value = BoltList_value(servers_value, i);
        if (server_value==NULL || BoltValue_type(server_value)!=BOLT_DICTIONARY) {
            status = BOLT_ROUTING_UNEXPECTED_DISCOVERY_RESPONSE;
            break;
        }

        struct BoltValue* role_value = BoltDictionary_value_by_key(server_value, ROLE_KEY, ROLE_KEY_LEN);
        if (role_value==NULL || BoltValue_type(role_value)!=BOLT_STRING) {
            status = BOLT_ROUTING_UNEXPECTED_DISCOVERY_RESPONSE;
            break;
        }

        struct BoltValue* addresses_value = BoltDictionary_value_by_key(server_value, ADDRESSES_KEY, ADDRESSES_KEY_LEN);
        if (addresses_value==NULL || BoltValue_type(addresses_value)!=BOLT_LIST) {
            status = BOLT_ROUTING_UNEXPECTED_DISCOVERY_RESPONSE;
            break;
        }

        if (status==BOLT_SUCCESS) {
            char* role = BoltString_get(role_value);

            for (int32_t j = 0; j<addresses_value->size && status==BOLT_SUCCESS; j++) {
                struct BoltValue* address_value = BoltList_value(addresses_value, j);
                if (address_value==NULL || BoltValue_type(address_value)!=BOLT_STRING) {
                    status = BOLT_ROUTING_UNEXPECTED_DISCOVERY_RESPONSE;
                    continue;
                }

                struct BoltAddress* address = BoltAddress_create_from_string(BoltString_get(address_value),
                        address_value->size);

                if (strcmp(role, READ_ROLE)==0) {
                    BoltAddressSet_add(readers, *address);
                }
                else if (strcmp(role, WRITE_ROLE)==0) {
                    BoltAddressSet_add(writers, *address);
                }
                else if (strcmp(role, ROUTE_ROLE)==0) {
                    BoltAddressSet_add(routers, *address);
                }
                else {
                    status = BOLT_ROUTING_UNEXPECTED_DISCOVERY_RESPONSE;
                }

                BoltAddress_destroy(address);
            }
        }
    }

    if (status==BOLT_SUCCESS) {
        BoltAddressSet_replace(table->readers, readers);
        BoltAddressSet_replace(table->writers, writers);
        BoltAddressSet_replace(table->routers, routers);
        table->last_updated = BoltUtil_get_time_ms();
        table->expires = table->last_updated+ttl;
    }

    BoltAddressSet_destroy(readers);
    BoltAddressSet_destroy(writers);
    BoltAddressSet_destroy(routers);

    return status;
}

int RoutingTable_is_expired(struct RoutingTable* state, enum BoltAccessMode mode)
{
    return state->routers->size==0
            || (mode==BOLT_ACCESS_MODE_READ ? state->readers->size==0 : state->writers->size==0)
            || state->expires<=BoltUtil_get_time_ms();
}

void RoutingTable_forget_server(struct RoutingTable* state, struct BoltAddress address)
{
    BoltAddressSet_remove(state->routers, address);
    BoltAddressSet_remove(state->readers, address);
    BoltAddressSet_remove(state->writers, address);
}

void RoutingTable_forget_writer(struct RoutingTable* state, struct BoltAddress address)
{
    BoltAddressSet_remove(state->writers, address);
}