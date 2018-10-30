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

#include "bolt-private.h"

#include "address-private.h"
#include "config-private.h"
#include "connector-private.h"
#include "direct-pool.h"
#include "mem.h"
#include "routing-pool.h"
#include "status-private.h"

BoltConfig* BoltConnector_apply_defaults(BoltConfig* config)
{
    if (config->trust==NULL) {
        config->trust = BoltTrust_create();
    }
    if (config->socket_options==NULL) {
        config->socket_options = BoltSocketOptions_create();
    }
    return config;
}

BoltConnector*
BoltConnector_create(BoltAddress* address, BoltValue* auth_token, struct BoltConfig* config)
{
    BoltConnector* connector = (BoltConnector*) BoltMem_allocate(sizeof(BoltConnector));
    connector->address = BoltAddress_create(address->host, address->port);
    connector->auth_token = BoltValue_duplicate(auth_token);
    connector->config = BoltConnector_apply_defaults(BoltConfig_clone(config));

    switch (connector->config->mode) {
    case BOLT_MODE_DIRECT:
        connector->pool_state = BoltDirectPool_create(address, connector->auth_token, connector->config);
        break;
    case BOLT_MODE_ROUTING:
        connector->pool_state = BoltRoutingPool_create(address, connector->auth_token, connector->config);
        break;
    default:
        // TODO: Set some status
        connector->pool_state = NULL;
    }

    return connector;
}

void BoltConnector_destroy(BoltConnector* connector)
{
    switch (connector->config->mode) {
    case BOLT_MODE_DIRECT:
        BoltDirectPool_destroy((struct BoltDirectPool*) connector->pool_state);
        break;
    case BOLT_MODE_ROUTING:
        BoltRoutingPool_destroy((struct BoltRoutingPool*) connector->pool_state);
        break;
    }

    BoltConfig_destroy((struct BoltConfig*) connector->config);
    BoltAddress_destroy((BoltAddress*) connector->address);
    BoltValue_destroy((BoltValue*) connector->auth_token);
    BoltMem_deallocate(connector, sizeof(BoltConnector));
}

BoltConnection* BoltConnector_acquire(BoltConnector* connector, BoltAccessMode mode, BoltStatus *status)
{
    switch (connector->config->mode) {
    case BOLT_MODE_DIRECT:
        return BoltDirectPool_acquire((struct BoltDirectPool*) connector->pool_state, status);
    case BOLT_MODE_ROUTING:
        return BoltRoutingPool_acquire((struct BoltRoutingPool*) connector->pool_state, mode, status);
    }

    status->state = BOLT_CONNECTION_STATE_DISCONNECTED;
    status->error = BOLT_UNSUPPORTED;
    status->error_ctx = NULL;
    status->error_ctx_size = 0;

    return NULL;
}

void BoltConnector_release(BoltConnector* connector, BoltConnection* connection)
{
    switch (connector->config->mode) {
    case BOLT_MODE_DIRECT:
        BoltDirectPool_release((struct BoltDirectPool*) connector->pool_state, connection);
        break;
    case BOLT_MODE_ROUTING:
        BoltRoutingPool_release((struct BoltRoutingPool*) connector->pool_state, connection);
        break;
    }
}
