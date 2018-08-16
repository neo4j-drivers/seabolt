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
#include "pooling/direct_pool.h"
#include "pooling/routing_pool.h"

#define SIZE_OF_CONNECTOR sizeof(struct BoltConnector)
#define SIZE_OF_CONFIG sizeof(struct BoltConfig)

struct BoltConnector* BoltConnector_create(struct BoltAddress* address, struct BoltConfig* config)
{
    struct BoltConfig* config_clone = (struct BoltConfig*) BoltMem_allocate(SIZE_OF_CONFIG);
    config_clone->mode = config->mode;
    config_clone->transport = config->transport;
    config_clone->user_agent = (char*) BoltMem_duplicate(config->user_agent, SIZE_OF_C_STRING(config->user_agent));
    config_clone->auth_token = BoltValue_duplicate(config->auth_token);
    config_clone->routing_context = BoltValue_duplicate(config->routing_context);
    config_clone->max_pool_size = config->max_pool_size;

    struct BoltConnector* connector = (struct BoltConnector*) BoltMem_allocate(SIZE_OF_CONNECTOR);
    connector->config = config_clone;
    connector->address = BoltAddress_create(address->host, address->port);

    switch (config_clone->mode) {
    case BOLT_DIRECT:
        connector->pool_state = BoltConnectionPool_create(address, config_clone);
        break;
    case BOLT_ROUTING:
        connector->pool_state = BoltRoutingConnectionPool_create(address, config_clone);
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
        BoltConnectionPool_destroy((struct BoltConnectionPool*) connector->pool_state);
        break;
    case BOLT_ROUTING:
        BoltRoutingConnectionPool_destroy((struct BoltRoutingConnectionPool*) connector->pool_state);
        break;
    }

    BoltAddress_destroy((struct BoltAddress*) connector->address);
    BoltMem_deallocate((char*) connector->config->user_agent, SIZE_OF_C_STRING(connector->config->user_agent));
    BoltValue_destroy((struct BoltValue*) connector->config->auth_token);
    BoltValue_destroy((struct BoltValue*) connector->config->routing_context);
    BoltMem_deallocate((void*) connector->config, SIZE_OF_CONFIG);
    BoltMem_deallocate(connector, SIZE_OF_CONNECTOR);
}

struct BoltConnectionResult BoltConnector_acquire(struct BoltConnector* connector, enum BoltAccessMode mode)
{
    switch (connector->config->mode) {
    case BOLT_DIRECT:
        return BoltConnectionPool_acquire((struct BoltConnectionPool*) connector->pool_state);
    case BOLT_ROUTING:
        return BoltRoutingConnectionPool_acquire((struct BoltRoutingConnectionPool*) connector->pool_state, mode);
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
        BoltConnectionPool_release((struct BoltConnectionPool*) connector->pool_state, connection);
        break;
    case BOLT_ROUTING:
        BoltRoutingConnectionPool_release((struct BoltRoutingConnectionPool*) connector->pool_state, connection);
        break;
    }
}
