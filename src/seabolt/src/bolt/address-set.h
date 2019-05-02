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
#ifndef SEABOLT_ALL_ADDRESS_SET_H
#define SEABOLT_ALL_ADDRESS_SET_H

#include "bolt-public.h"
#include "address.h"

/**
 * The type that represents a set of \ref BoltAddress instances.
 *
 * It is the destination data structure for a custom \ref BoltAddressResolver to provide its resolved
 * addresses.
 */
typedef struct BoltAddressSet BoltAddressSet;

/**
 * Adds a \ref BoltAddress instance to the set, if it's not present.
 *
 * @param set the target set instance.
 * @param address the address instance to be added to the set.
 * @returns -1 if the provided address is already a member of the set, or the index at which the given address
 * is added.
 */
SEABOLT_EXPORT int32_t BoltAddressSet_add(volatile BoltAddressSet* set, volatile const BoltAddress* address);

#endif //SEABOLT_ALL_ADDRESS_SET_H
