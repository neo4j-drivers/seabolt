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
#ifndef SEABOLT_ALL_ADDRESS_SET_H
#define SEABOLT_ALL_ADDRESS_SET_H

#include "bolt-public.h"
#include "address.h"

typedef struct BoltAddressSet BoltAddressSet;

SEABOLT_EXPORT BoltAddressSet* BoltAddressSet_create();

SEABOLT_EXPORT void BoltAddressSet_destroy(BoltAddressSet* set);

SEABOLT_EXPORT int BoltAddressSet_size(BoltAddressSet* set);

SEABOLT_EXPORT int BoltAddressSet_index_of(BoltAddressSet* set, const BoltAddress* address);

SEABOLT_EXPORT int BoltAddressSet_add(BoltAddressSet* set, const BoltAddress* address);

SEABOLT_EXPORT int BoltAddressSet_remove(BoltAddressSet* set, const BoltAddress* address);

SEABOLT_EXPORT void BoltAddressSet_replace(BoltAddressSet* destination, BoltAddressSet* source);

SEABOLT_EXPORT void BoltAddressSet_add_all(BoltAddressSet* destination, BoltAddressSet* source);

#endif //SEABOLT_ALL_ADDRESS_SET_H
