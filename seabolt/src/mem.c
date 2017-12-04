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

#include <stddef.h>
#include <malloc.h>

#include "mem.h"


static size_t __memory = 0;

void* BoltMem_allocate(size_t new_size)
{
    void* p = malloc(new_size);
    __memory += new_size;
    fprintf(stderr, "\x1B[36mAllocated %ld bytes (balance: %lu)\x1B[0m\n", new_size, __memory);
    return p;
}

void* BoltMem_reallocate(void* ptr, size_t old_size, size_t new_size)
{
    void* p = realloc(ptr, new_size);
    __memory = __memory - old_size + new_size;
    fprintf(stderr, "\x1B[36mReallocated %ld bytes as %ld bytes (balance: %lu)\x1B[0m\n", old_size, new_size, __memory);
    return p;
}

void* BoltMem_deallocate(void* ptr, size_t old_size)
{
    free(ptr);
    __memory -= old_size;
    fprintf(stderr, "\x1B[36mFreed %ld bytes (balance: %lu)\x1B[0m\n", old_size, __memory);
    return NULL;
}
