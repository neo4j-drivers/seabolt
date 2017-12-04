/*
 * Copyright (c) 2002-2017 "Neo Technology,"
 * Network Engine for Objects in Lund AB [http://neotechnology.com]
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

#ifndef SEABOLT_MEMORY
#define SEABOLT_MEMORY


/**
 * Allocate memory.
 *
 * @param new_size
 * @return
 */
void* BoltMem_allocate(size_t new_size);

/**
 * Reallocate memory.
 *
 * @param ptr
 * @param old_size
 * @param new_size
 * @return
 */
void* BoltMem_reallocate(void* ptr, size_t old_size, size_t new_size);

/**
 * Deallocate memory.
 *
 * @param ptr
 * @param old_size
 * @return
 */
void* BoltMem_deallocate(void* ptr, size_t old_size);

/**
 * Get an activity count for memory (de/re/-)allocation
 * @return
 */
long long BoltMem_activity();


#endif // SEABOLT_MEMORY
