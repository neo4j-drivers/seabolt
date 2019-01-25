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

#include "bolt-private.h"
#include "communication-secure.h"
#include "config-private.h"
#include "log-private.h"
#include "status-private.h"
#include "mem.h"
#include "communication-plain.h"

typedef struct BoltSecurityContext {
    void* empty;
} BoltSecurityContext;

BoltSecurityContext*
BoltSecurityContext_create(struct BoltTrust* trust, const char* hostname, const struct BoltLog* log)
{
    UNUSED(trust);
    UNUSED(hostname);
    UNUSED(log);
    return NULL;
}

void BoltSecurityContext_destroy(BoltSecurityContext* context)
{
    UNUSED(context);
}

int BoltSecurityContext_startup()
{
    return BOLT_SUCCESS;
}

int BoltSecurityContext_shutdown()
{
    return BOLT_SUCCESS;
}

BoltCommunication* BoltCommunication_create_secure(BoltSecurityContext* sec_ctx, BoltTrust* trust,
        BoltSocketOptions* socket_options, BoltLog* log, const char* hostname, const char* id)
{
    UNUSED(sec_ctx);
    UNUSED(trust);
    UNUSED(hostname);
    UNUSED(id);
    BoltCommunication* plain_comm = BoltCommunication_create_plain(socket_options, log);

    return plain_comm;
}
