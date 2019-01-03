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
#ifndef SEABOLT_STATS_H
#define SEABOLT_STATS_H

#include "bolt-public.h"

/**
 * Returns the current allocated memory by the internal connector data structures.
 *
 * @returns the current allocated memory
 */
SEABOLT_EXPORT uint64_t BoltStat_memory_allocation_current();

/**
 * Returns the peak allocated memory by the internal connector data structures.
 *
 * @return the peak allocated memory
 */
SEABOLT_EXPORT uint64_t BoltStat_memory_allocation_peak();

/**
 * Returns the number of allocation events (malloc, realloc, free) by the internal
 * connector data structures.
 *
 * @return the number of allocation events.
 */
SEABOLT_EXPORT int64_t BoltStat_memory_allocation_events();

#endif //SEABOLT_STATS_H
