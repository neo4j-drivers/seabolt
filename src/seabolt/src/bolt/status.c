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
#include "status-private.h"
#include "mem.h"

BoltStatus* BoltStatus_create()
{
    return BoltStatus_create_with_ctx(0);
}

BoltStatus* BoltStatus_create_with_ctx(uint64_t context_size)
{
    BoltStatus* status = BoltMem_allocate(sizeof(BoltStatus));
    status->state = BOLT_CONNECTION_STATE_DISCONNECTED;
    status->error = BOLT_SUCCESS;
    status->error_ctx = context_size ? BoltMem_allocate(context_size) : NULL;
    status->error_ctx_size = context_size;
    return status;
}

void BoltStatus_destroy(BoltStatus* status)
{
    if (status->error_ctx_size>0) {
        BoltMem_deallocate(status->error_ctx, status->error_ctx_size);
    }
    BoltMem_deallocate(status, sizeof(BoltStatus));
}

BoltConnectionState BoltStatus_get_state(BoltStatus* status)
{
    return status->state;
}

int32_t BoltStatus_get_error(BoltStatus* status)
{
    return status->error;
}

const char* BoltStatus_get_error_context(BoltStatus* status)
{
    return status->error_ctx;
}
