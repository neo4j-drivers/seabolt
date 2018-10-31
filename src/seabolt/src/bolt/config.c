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
#include "config-private.h"
#include "address-resolver-private.h"
#include "log-private.h"
#include "mem.h"

BoltSocketOptions* BoltSocketOptions_create()
{
    BoltSocketOptions* socket_options = BoltMem_allocate(sizeof(BoltSocketOptions));
    socket_options->connect_timeout = 5000;
    socket_options->recv_timeout = 0;
    socket_options->send_timeout = 0;
    socket_options->keep_alive = 1;
    return socket_options;
}

BoltSocketOptions* BoltSocketOptions_clone(BoltSocketOptions* socket_options)
{
    if (socket_options==NULL) {
        return NULL;
    }

    BoltSocketOptions* clone = BoltSocketOptions_create();
    clone->connect_timeout = socket_options->connect_timeout;
    clone->recv_timeout = socket_options->recv_timeout;
    clone->send_timeout = socket_options->send_timeout;
    clone->keep_alive = socket_options->keep_alive;
    return clone;
}

void BoltSocketOptions_destroy(BoltSocketOptions* socket_options)
{
    BoltMem_deallocate(socket_options, sizeof(BoltSocketOptions));
}

int32_t BoltSocketOptions_get_connect_timeout(BoltSocketOptions* socket_options)
{
    return socket_options->connect_timeout;
}

int32_t BoltSocketOptions_set_connect_timeout(BoltSocketOptions* socket_options, int32_t connect_timeout)
{
    socket_options->connect_timeout = connect_timeout;
    return BOLT_SUCCESS;
}

int32_t BoltSocketOptions_get_keep_alive(BoltSocketOptions* socket_options)
{
    return socket_options->keep_alive;
}

int32_t BoltSocketOptions_set_keep_alive(BoltSocketOptions* socket_options, int32_t keep_alive)
{
    socket_options->keep_alive = keep_alive;
    return BOLT_SUCCESS;
}

BoltTrust* BoltTrust_create()
{
    BoltTrust* trust = BoltMem_allocate(sizeof(BoltTrust));
    trust->certs = NULL;
    trust->certs_len = 0;
    trust->skip_verify = 1;
    trust->skip_verify_hostname = 1;
    return trust;
}

BoltTrust* BoltTrust_clone(BoltTrust* source)
{
    if (source==NULL) {
        return NULL;
    }

    BoltTrust* clone = BoltTrust_create();
    BoltTrust_set_certs(clone, source->certs, source->certs_len);
    BoltTrust_set_skip_verify(clone, source->skip_verify);
    BoltTrust_set_skip_verify_hostname(clone, source->skip_verify_hostname);
    return clone;
}

void BoltTrust_destroy(BoltTrust* trust)
{
    if (trust->certs!=NULL) {
        BoltMem_deallocate(trust->certs, trust->certs_len);
    }
    BoltMem_deallocate(trust, sizeof(BoltTrust));
}

const char* BoltTrust_get_certs(BoltTrust* trust, uint64_t* size)
{
    *size = trust->certs_len;
    return trust->certs;
}

int32_t BoltTrust_set_certs(BoltTrust* trust, const char* certs_pem, uint64_t certs_pem_size)
{
    trust->certs = BoltMem_duplicate(certs_pem, certs_pem_size);
    trust->certs_len = certs_pem_size;
    return BOLT_SUCCESS;
}

int32_t BoltTrust_get_skip_verify(BoltTrust* trust)
{
    return trust->skip_verify;
}

int32_t BoltTrust_set_skip_verify(BoltTrust* trust, int32_t skip_verify)
{
    trust->skip_verify = skip_verify;
    return BOLT_SUCCESS;
}

int32_t BoltTrust_get_skip_verify_hostname(BoltTrust* trust)
{
    return trust->skip_verify_hostname;
}

int32_t BoltTrust_set_skip_verify_hostname(BoltTrust* trust, int32_t skip_verify_hostname)
{
    trust->skip_verify_hostname = skip_verify_hostname;
    return BOLT_SUCCESS;
}

BoltConfig* BoltConfig_create()
{
    BoltConfig* config = BoltMem_allocate(sizeof(BoltConfig));
    config->mode = BOLT_MODE_DIRECT;
    config->transport = BOLT_TRANSPORT_ENCRYPTED;
    config->trust = NULL;
    config->user_agent = NULL;
    config->routing_context = NULL;
    config->address_resolver = NULL;
    config->log = NULL;
    config->max_pool_size = 100;
    config->max_connection_life_time = 0;
    config->max_connection_acquisition_time = 0;
    config->socket_options = NULL;
    return config;
}

BoltConfig* BoltConfig_clone(BoltConfig* config)
{
    BoltConfig* clone = BoltConfig_create();
    if (config!=NULL) {
        BoltConfig_set_mode(clone, config->mode);
        BoltConfig_set_transport(clone, config->transport);
        BoltConfig_set_trust(clone, config->trust);
        BoltConfig_set_user_agent(clone, config->user_agent);
        BoltConfig_set_routing_context(clone, config->routing_context);
        BoltConfig_set_address_resolver(clone, config->address_resolver);
        BoltConfig_set_log(clone, config->log);
        BoltConfig_set_max_pool_size(clone, config->max_pool_size);
        BoltConfig_set_max_connection_life_time(clone, config->max_connection_life_time);
        BoltConfig_set_max_connection_acquisition_time(clone, config->max_connection_acquisition_time);
        BoltConfig_set_socket_options(clone, config->socket_options);
    }
    return clone;
}

void BoltConfig_destroy(BoltConfig* config)
{
    if (config->trust!=NULL) {
        BoltTrust_destroy(config->trust);
    }
    if (config->user_agent!=NULL) {
        BoltMem_deallocate(config->user_agent, SIZE_OF_C_STRING(config->user_agent));
    }
    if (config->routing_context!=NULL) {
        BoltValue_destroy(config->routing_context);
    }
    if (config->address_resolver!=NULL) {
        BoltAddressResolver_destroy(config->address_resolver);
    }
    if (config->log!=NULL) {
        BoltLog_destroy(config->log);
    }
    if (config->socket_options!=NULL) {
        BoltSocketOptions_destroy(config->socket_options);
    }
    BoltMem_deallocate(config, sizeof(BoltConfig));
}

BoltMode BoltConfig_get_mode(BoltConfig* config)
{
    return config->mode;
}

int32_t BoltConfig_set_mode(BoltConfig* config, BoltMode mode)
{
    config->mode = mode;
    return BOLT_SUCCESS;
}

BoltTransport BoltConfig_get_transport(BoltConfig* config)
{
    return config->transport;
}

int32_t BoltConfig_set_transport(BoltConfig* config, BoltTransport transport)
{
    config->transport = transport;
    return BOLT_SUCCESS;
}

BoltTrust* BoltConfig_get_trust(BoltConfig* config)
{
    return config->trust;
}

int32_t BoltConfig_set_trust(BoltConfig* config, BoltTrust* trust)
{
    config->trust = BoltTrust_clone(trust);
    return BOLT_SUCCESS;
}

const char* BoltConfig_get_user_agent(BoltConfig* config)
{
    return config->user_agent;
}

int32_t BoltConfig_set_user_agent(BoltConfig* config, const char* user_agent)
{
    config->user_agent = BoltMem_duplicate(user_agent, SIZE_OF_C_STRING(user_agent));
    return BOLT_SUCCESS;
}

BoltValue* BoltConfig_get_routing_context(BoltConfig* config)
{
    return config->routing_context;
}

int32_t BoltConfig_set_routing_context(BoltConfig* config, BoltValue* routing_context)
{
    config->routing_context = BoltValue_duplicate(routing_context);
    return BOLT_SUCCESS;
}

BoltAddressResolver* BoltConfig_get_address_resolver(BoltConfig* config)
{
    return config->address_resolver;
}

int32_t BoltConfig_set_address_resolver(BoltConfig* config, BoltAddressResolver* address_resolver)
{
    config->address_resolver = BoltAddressResolver_clone(address_resolver);
    return BOLT_SUCCESS;
}

BoltLog* BoltConfig_get_log(BoltConfig* config)
{
    return config->log;
}

int32_t BoltConfig_set_log(BoltConfig* config, BoltLog* log)
{
    config->log = BoltLog_clone(log);
    return BOLT_SUCCESS;
}

int32_t BoltConfig_get_max_pool_size(BoltConfig* config)
{
    return config->max_pool_size;
}

int32_t BoltConfig_set_max_pool_size(BoltConfig* config, int32_t max_pool_size)
{
    config->max_pool_size = max_pool_size;
    return BOLT_SUCCESS;
}

int32_t BoltConfig_get_max_connection_life_time(BoltConfig* config)
{
    return config->max_connection_life_time;
}

int32_t BoltConfig_set_max_connection_life_time(BoltConfig* config, int32_t max_connection_life_time)
{
    config->max_connection_life_time = max_connection_life_time;
    return BOLT_SUCCESS;
}

int32_t BoltConfig_get_max_connection_acquisition_time(BoltConfig* config)
{
    return config->max_connection_acquisition_time;
}

int32_t BoltConfig_set_max_connection_acquisition_time(BoltConfig* config, int32_t max_connection_acquisition_time)
{
    config->max_connection_acquisition_time = max_connection_acquisition_time;
    return BOLT_SUCCESS;
}

BoltSocketOptions* BoltConfig_get_socket_options(BoltConfig* config)
{
    return config->socket_options;
}

int32_t BoltConfig_set_socket_options(BoltConfig* config, BoltSocketOptions* socket_options)
{
    config->socket_options = BoltSocketOptions_clone(socket_options);
    return BOLT_SUCCESS;
}
