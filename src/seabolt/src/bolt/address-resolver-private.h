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
#ifndef SEABOLT_ADDRESS_RESOLVER_PRIVATE_H
#define SEABOLT_ADDRESS_RESOLVER_PRIVATE_H

#include "address-resolver.h"

struct BoltAddressResolver {
    void* state;
    address_resolver_func resolver;
};

BoltAddressResolver* BoltAddressResolver_clone(BoltAddressResolver* resolver);

void BoltAddressResolver_resolve(BoltAddressResolver* resolver, const BoltAddress* address,
        volatile BoltAddressSet* resolved);

#endif //SEABOLT_ADDRESS_RESOLVER_PRIVATE_H
