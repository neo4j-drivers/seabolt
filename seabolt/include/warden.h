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

/// This module....
/// Logging, resource management, "try" and exception macros
/// General flow management

#ifndef SEABOLT_WARDEN
#define SEABOLT_WARDEN

#include "stdio.h"


static FILE* __bolt_log_file;

void BoltLog_setFile(FILE* log_file);

void BoltLog_info(const char* message, ...);

void BoltLog_error(const char* message, ...);



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
 * Allocate, reallocate or free memory for data storage.
 *
 * Since we recycle values, we can also potentially recycle
 * the dynamically-allocated storage.
 *
 * @param ptr a pointer to the existing memory (if any)
 * @param old_size the number of bytes already allocated
 * @param new_size the new number of bytes required
 */
void* BoltMem_adjust(void* ptr, size_t old_size, size_t new_size);

/**
 * Retrieve the amount of memory currently allocated.
 *
 * @return
 */
size_t BoltMem_allocated();

/**
 * Get an activity count for memory (de/re/-)allocation.
 *
 * @return
 */
long long BoltMem_activity();


#endif // SEABOLT_WARDEN
