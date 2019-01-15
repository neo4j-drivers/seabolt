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

#ifndef SEABOLT_ATOMIC_H
#define SEABOLT_ATOMIC_H

#include "bolt-public.h"

int64_t BoltAtomic_increment(volatile int64_t* ref);

int64_t BoltAtomic_decrement(volatile int64_t* ref);

int64_t BoltAtomic_add(volatile int64_t* ref, int64_t by);

#endif //SEABOLT_ATOMIC_H

