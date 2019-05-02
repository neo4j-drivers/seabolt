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
#ifndef SEABOLT_ADDRESS_SET_PRIVATE_H
#define SEABOLT_ADDRESS_SET_PRIVATE_H

#include "address-set.h"

struct BoltAddressSet {
    volatile int32_t size;
    volatile BoltAddress** elements;
};

#define SIZE_OF_ADDRESS_SET sizeof(struct BoltAddressSet)
#define SIZE_OF_ADDRESS_SET_PTR sizeof(struct BoltAddressSet*)

volatile BoltAddressSet* BoltAddressSet_create();

void BoltAddressSet_destroy(volatile BoltAddressSet* set);

int32_t BoltAddressSet_size(volatile BoltAddressSet* set);

int32_t BoltAddressSet_index_of(volatile BoltAddressSet* set, volatile const BoltAddress* address);

int32_t BoltAddressSet_remove(volatile BoltAddressSet* set, volatile const BoltAddress* address);

void BoltAddressSet_replace(volatile BoltAddressSet* destination, volatile BoltAddressSet* source);

void BoltAddressSet_add_all(volatile BoltAddressSet* destination, volatile BoltAddressSet* source);

#endif //SEABOLT_ADDRESS_SET_PRIVATE_H
