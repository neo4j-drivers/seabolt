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


extern "C"
{
#include <bolt/connections.h>
#include "bolt/platform.h"

void BoltLog_info(const char* message, ...)
{

}

int BoltUtil_get_time(struct timespec* t)
{
    return 0;
}

int64_t BoltUtil_get_time_ms()
{
    return 0;
}

int BoltUtil_mutex_create(mutex_t* mutex)
{
    return 0;
}

int BoltUtil_mutex_destroy(mutex_t* mutex)
{
    return 0;
}

int BoltUtil_mutex_lock(mutex_t* mutex)
{
    return 0;
}

int BoltUtil_mutex_unlock(mutex_t* mutex)
{
    return 0;
}

const char* BoltProtocolV1_structure_name(int16_t code)
{
    return "MOCKED";
}

int BoltConnection_open(struct BoltConnection* connection, enum BoltTransport transport,
        struct BoltAddress* address)
{
    return 0;
}

void BoltConnection_close(struct BoltConnection* connection)
{

}

int
BoltConnection_init(struct BoltConnection* connection, const char* user_agent, const struct BoltValue* auth_token)
{
    return 0;
}

int BoltConnection_load_reset_request(struct BoltConnection* connection)
{
    return 0;
}

bolt_request_t BoltConnection_last_request(struct BoltConnection* connection)
{
    return 0;
}

int BoltConnection_send(struct BoltConnection* connection)
{
    return 0;
}

int BoltConnection_fetch_summary(struct BoltConnection* connection, bolt_request_t request)
{
    return 0;
}

int BoltConnection_summary_success(struct BoltConnection* connection)
{
    return 0;
}

int BoltConnection_cypher(struct BoltConnection* connection, const char* cypher, size_t cypher_size,
        int32_t n_parameters)
{
    return 0;
}

struct BoltValue* BoltConnection_cypher_parameter(struct BoltConnection* connection, int32_t index,
        const char* key, size_t key_size)
{
    return BoltValue_create();
}

int BoltConnection_load_run_request(struct BoltConnection* connection)
{
    return 0;
}

int BoltConnection_load_pull_request(struct BoltConnection* connection, int32_t n)
{
    return 0;
}

struct BoltValue* BoltConnection_record_fields(struct BoltConnection* connection)
{
    return BoltValue_create();
}

struct BoltValue* BoltConnection_fields(struct BoltConnection* connection)
{
    return BoltValue_create();
}

int BoltConnection_fetch(struct BoltConnection* connection, bolt_request_t request)
{
    return 0;
}

}