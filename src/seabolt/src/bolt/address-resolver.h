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
#ifndef SEABOLT_ALL_SERVER_ADDRESS_RESOLVER_H
#define SEABOLT_ALL_SERVER_ADDRESS_RESOLVER_H

#include "bolt-public.h"
#include "address.h"
#include "address-set.h"

/**
 * The address resolver function signature that implementors should provide for a working custom resolver.
 *
 * @param state the state object as provided to \ref BoltAddressResolver_create.
 * @param address the address to be resolved by the resolver function.
 * @param set the target set the resolved addresses should be added.
 */
typedef void (* address_resolver_func)(void* state, BoltAddress* address, BoltAddressSet* set);

/**
 * The type that defines an address resolver to be used during the initial (and fall back) routing discovery.
 *
 * An instance needs to be created with \ref BoltAddressResolver_create and destroyed with
 * \ref BoltAddressResolver_destroy after \ref BoltConfig_set_address_resolver is called.
 */
typedef struct BoltAddressResolver BoltAddressResolver;

/**
 * Creates a new instance of \ref BoltAddressResolver.
 *
 * @param state an optional object that will be passed to provided address resolver function.
 * @param resolver_func the address resolver function to be called.
 * @returns the pointer to the newly allocated \ref BoltAddressResolver instance.
 */
SEABOLT_EXPORT BoltAddressResolver* BoltAddressResolver_create(void* state, address_resolver_func resolver_func);

/**
 * Destroys the passed \ref BoltAddressResolver instance.
 *
 * @param resolver the instance to be destroyed.
 */SEABOLT_EXPORT void BoltAddressResolver_destroy(BoltAddressResolver* resolver);

#endif //SEABOLT_ALL_SERVER_ADDRESS_RESOLVER_H
