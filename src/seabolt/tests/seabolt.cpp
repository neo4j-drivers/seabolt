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



#include <cstring>
#include <cstdlib>
#include <integration.hpp>
#include "catch.hpp"

void log_to_stderr(void* state, const char* message)
{
    UNUSED(state);
    fprintf(stderr, "%s\n", message);
}

struct BoltLog* create_logger()
{
    struct BoltLog* log = BoltLog_create(0);
    int debug = 0;
    int warn = 0;
    int info = 0;
    int error = 0;

    if (strcmp(BOLT_LOG, "error")==0) {
        error = 1;
    }

    if (strcmp(BOLT_LOG, "warning")==0) {
        error = 1;
        warn = 1;
    }

    if (strcmp(BOLT_LOG, "info")==0) {
        error = 1;
        warn = 1;
        info = 1;
    }

    if (strcmp(BOLT_LOG, "debug")==0) {
        error = 1;
        warn = 1;
        info = 1;
        debug = 1;
    }

    BoltLog_set_debug_func(log, debug ? log_to_stderr : NULL);
    BoltLog_set_warning_func(log, warn ? log_to_stderr : NULL);
    BoltLog_set_info_func(log, info ? log_to_stderr : NULL);
    BoltLog_set_error_func(log, error ? log_to_stderr : NULL);
    return log;
}

struct BoltAddress* bolt_get_address(const char* host, const char* port)
{
    struct BoltAddress* address = BoltAddress_create(host, port);
    int status = BoltAddress_resolve(address, NULL, NULL);
    REQUIRE(status==0);
    return address;
}

struct BoltConnection* bolt_open_b(BoltTransport transport, const char* host, const char* port)
{
    struct BoltTrust trust{nullptr, 0, 1, 1};
    struct BoltAddress* address = bolt_get_address(host, port);
    struct BoltConnection* connection = BoltConnection_create();
    BoltConnection_open(connection, transport, address, &trust, create_logger(), NULL);
    BoltAddress_destroy(address);
    REQUIRE(connection->status->state==BOLT_CONNECTION_STATE_CONNECTED);
    return connection;
}

struct BoltConnection* bolt_open_init_b(BoltTransport transport, const char* host, const char* port,
        const char* user_agent, const struct BoltValue* auth_token)
{
    struct BoltConnection* connection = bolt_open_b(transport, host, port);
    BoltConnection_init(connection, user_agent, auth_token);
    REQUIRE(connection->status->state==BOLT_CONNECTION_STATE_READY);
    return connection;
}

struct BoltConnection* bolt_open_init_default()
{
    struct BoltValue* auth_token = BoltAuth_basic(BOLT_USER, BOLT_PASSWORD, NULL);
    struct BoltConnection* connection = bolt_open_init_b(BOLT_TRANSPORT_ENCRYPTED, BOLT_IPV6_HOST, BOLT_PORT,
            BOLT_USER_AGENT, auth_token);
    BoltValue_destroy(auth_token);
    return connection;
}

void bolt_close_and_destroy_b(struct BoltConnection* connection)
{
    BoltConnection_close(connection);
    if (connection->log!=NULL) {
        BoltLog_destroy((BoltLog*) connection->log);
    }
    BoltConnection_destroy(connection);
}
