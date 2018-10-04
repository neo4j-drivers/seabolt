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


extern "C" {
#include <bolt/connections.h>
#include "bolt/platform.h"

#if defined(_WIN32) && _MSC_VER
#pragma warning( push )
#pragma warning( disable : 4100 )
#endif

int BoltUtil_get_time(struct timespec* t)
{
    return 0;
}

int64_t BoltUtil_get_time_ms()
{
    return 0;
}

int64_t BoltUtil_increment(volatile int64_t* ref)
{
    return (*ref += 1);
}

int64_t BoltUtil_decrement(volatile int64_t* ref)
{
    return (*ref -= 1);
}

int BoltUtil_mutex_create(mutex_t* mutex)
{
    return 1;
}

int BoltUtil_mutex_destroy(mutex_t* mutex)
{
    return 1;
}

int BoltUtil_mutex_lock(mutex_t* mutex)
{
    return 1;
}

int BoltUtil_mutex_unlock(mutex_t* mutex)
{
    return 1;
}

int BoltUtil_rwlock_create(rwlock_t* rwlock)
{
    return 1;
}

int BoltUtil_rwlock_destroy(rwlock_t* rwlock)
{
    return 1;
}

int BoltUtil_rwlock_rdlock(rwlock_t* rwlock)
{
    return 1;
}

int BoltUtil_rwlock_wrlock(rwlock_t* rwlock)
{
    return 1;
}

int BoltUtil_rwlock_tryrdlock(rwlock_t* rwlock)
{
    return 1;
}

int BoltUtil_rwlock_trywrlock(rwlock_t* rwlock)
{
    return 1;
}

int BoltUtil_rwlock_timedrdlock(rwlock_t* rwlock, int timeout_ms)
{
    return 1;
}

int BoltUtil_rwlock_timedwrlock(rwlock_t* rwlock, int timeout_ms)
{
    return 1;
}

int BoltUtil_rwlock_rdunlock(rwlock_t* rwlock)
{
    return 1;
}

int BoltUtil_rwlock_wrunlock(rwlock_t* rwlock)
{
    return 1;
}

const char* BoltProtocolV1_structure_name(int16_t code)
{
    return "MOCKED";
}

const char* BoltProtocolV1_message_name(int16_t code)
{
    return "MOCKED";
}

int BoltConnection_open(struct BoltConnection* connection, enum BoltTransport transport,
        struct BoltAddress* address, struct BoltTrust* trust, struct BoltLog* log, struct BoltSocketOptions* sock_opts)
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

bolt_request BoltConnection_last_request(struct BoltConnection* connection)
{
    return 0;
}

int BoltConnection_send(struct BoltConnection* connection)
{
    return 0;
}

int BoltConnection_fetch_summary(struct BoltConnection* connection, bolt_request request)
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

struct BoltValue* BoltConnection_field_values(struct BoltConnection* connection)
{
    return BoltValue_create();
}

struct BoltValue* BoltConnection_field_names(struct BoltConnection* connection)
{
    return BoltValue_create();
}

int BoltConnection_fetch(struct BoltConnection* connection, bolt_request request)
{
    return 0;
}

#if defined(_WIN32) && _MSC_VER
#pragma warning( pop )
#endif

}
