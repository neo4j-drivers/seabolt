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
#include "bolt/mem.h"
#include "bolt/address-resolver.h"

struct BoltAddressResolver* BoltAddressResolver_create()
{
    struct BoltAddressResolver* resolver = (struct BoltAddressResolver*) BoltMem_allocate(
            sizeof(struct BoltAddressResolver));
    resolver->state = 0;
    resolver->resolver = NULL;
    return resolver;
}

void BoltAddressResolver_destroy(struct BoltAddressResolver* resolver)
{
    BoltMem_deallocate(resolver, sizeof(struct BoltAddressResolver));
}

void BoltAddressResolver_resolve(struct BoltAddressResolver* resolver, struct BoltAddress* address,
        struct BoltAddressSet* resolved)
{
    if (resolver!=NULL && resolver->resolver!=NULL) {
        resolver->resolver(resolver->state, address, resolved);
    }
}