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

#include "bolt/config-impl.h"
#include "bolt/connector.h"
#include "bolt/mem.h"
#include "pool/direct-pool.h"
#include "pool/routing-pool.h"

#define SIZE_OF_CONNECTOR sizeof(struct BoltConnector)
#define SIZE_OF_CONFIG sizeof(struct BoltConfig)
#define SIZE_OF_LOG sizeof(struct BoltLog)
#define SIZE_OF_ADDRESS_RESOLVER sizeof(struct BoltAddressResolver)

struct BoltConfig* BoltConfig_clone(struct BoltConfig* config)
{
    struct BoltConfig* clone = (struct BoltConfig*) BoltMem_allocate(SIZE_OF_CONFIG);
    clone->mode = config->mode;
    clone->transport = config->transport;
    clone->user_agent = (char*) BoltMem_duplicate(config->user_agent, SIZE_OF_C_STRING(config->user_agent));
    clone->routing_context = BoltValue_duplicate(config->routing_context);
    clone->max_pool_size = config->max_pool_size;
    clone->log = (struct BoltLog*) BoltMem_duplicate(config->log, SIZE_OF_LOG);
    clone->address_resolver = (struct BoltAddressResolver*) BoltMem_duplicate(config->address_resolver,
            SIZE_OF_ADDRESS_RESOLVER);
    return clone;
}

void BoltConfig_destroy(struct BoltConfig* config)
{
    BoltMem_deallocate((char*) config->user_agent, SIZE_OF_C_STRING(config->user_agent));
    BoltValue_destroy((struct BoltValue*) config->routing_context);
    BoltLog_destroy(config->log);
    BoltAddressResolver_destroy(config->address_resolver);
    BoltMem_deallocate(config, SIZE_OF_CONFIG);
}

struct BoltConnector*
BoltConnector_create(struct BoltAddress* address, struct BoltValue* auth_token, struct BoltConfig* config)
{
    struct BoltConnector* connector = (struct BoltConnector*) BoltMem_allocate(SIZE_OF_CONNECTOR);
    connector->address = BoltAddress_create(address->host, address->port);
    connector->auth_token = BoltValue_duplicate(auth_token);
    connector->config = BoltConfig_clone(config);;

    switch (connector->config->mode) {
    case BOLT_DIRECT:
        connector->pool_state = BoltDirectPool_create(address, connector->auth_token, connector->config);
        break;
    case BOLT_ROUTING:
        connector->pool_state = BoltRoutingPool_create(address, connector->auth_token, connector->config);
        break;
    default:
        // TODO: Set some status
        connector->pool_state = NULL;
    }

    return connector;
}

void BoltConnector_destroy(struct BoltConnector* connector)
{
    switch (connector->config->mode) {
    case BOLT_DIRECT:
        BoltDirectPool_destroy((struct BoltDirectPool*) connector->pool_state);
        break;
    case BOLT_ROUTING:
        BoltRoutingPool_destroy((struct BoltRoutingPool*) connector->pool_state);
        break;
    }

    BoltConfig_destroy((struct BoltConfig*) connector->config);
    BoltAddress_destroy((struct BoltAddress*) connector->address);
    BoltValue_destroy((struct BoltValue*) connector->auth_token);
    BoltMem_deallocate(connector, SIZE_OF_CONNECTOR);
}

struct BoltConnectionResult BoltConnector_acquire(struct BoltConnector* connector, enum BoltAccessMode mode)
{
    switch (connector->config->mode) {
    case BOLT_DIRECT:
        return BoltDirectPool_acquire((struct BoltDirectPool*) connector->pool_state);
    case BOLT_ROUTING:
        return BoltRoutingPool_acquire((struct BoltRoutingPool*) connector->pool_state, mode);
    }

    struct BoltConnectionResult result = {
            NULL, BOLT_DISCONNECTED, BOLT_UNSUPPORTED, NULL
    };
    return result;
}

void BoltConnector_release(struct BoltConnector* connector, struct BoltConnection* connection)
{
    switch (connector->config->mode) {
    case BOLT_DIRECT:
        BoltDirectPool_release((struct BoltDirectPool*) connector->pool_state, connection);
        break;
    case BOLT_ROUTING:
        BoltRoutingPool_release((struct BoltRoutingPool*) connector->pool_state, connection);
        break;
    }
}
