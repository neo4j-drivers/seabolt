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

#include "bolt-private.h"

#include "mem.h"
#include "atomic.h"

void* BoltMem_reverse_copy(void* dest, const void* src, int64_t n)
{
    char* dest_c = (char*) (dest);
    const char* src_c = (const char*) (src);
    for (int64_t i = 0; i<n; i++) {
        dest_c[i] = src_c[n-i-1];
    }
    return dest;
}

static int64_t __allocation = 0;
static int64_t __peak_allocation = 0;
static int64_t __allocation_events = 0;

void* BoltMem_allocate(int64_t new_size)
{
    void* p = malloc(new_size);
    int64_t new_allocation = BoltAtomic_add(&__allocation, new_size);
    int64_t peak_allocation = BoltAtomic_add(&__peak_allocation, 0);
    if (new_allocation>peak_allocation) BoltAtomic_add(&__peak_allocation, new_allocation-peak_allocation);
    BoltAtomic_increment(&__allocation_events);
    return p;
}

void* BoltMem_reallocate(void* ptr, int64_t old_size, int64_t new_size)
{
    void* p = realloc(ptr, new_size);

    int64_t new_allocation = BoltAtomic_add(&__allocation, -old_size+new_size);
    int64_t peak_allocation = BoltAtomic_add(&__peak_allocation, 0);
    if (__allocation>__peak_allocation) BoltAtomic_add(&__peak_allocation, new_allocation-peak_allocation);
    BoltAtomic_increment(&__allocation_events);
    return p;
}

void* BoltMem_deallocate(void* ptr, int64_t old_size)
{
    if (ptr==NULL) {
        return NULL;
    }

    free(ptr);
    BoltAtomic_add(&__allocation, -old_size);
    BoltAtomic_increment(&__allocation_events);
    return NULL;
}

void* BoltMem_adjust(void* ptr, int64_t old_size, int64_t new_size)
{
    if (new_size==old_size) {
        // In this case, the physical data storage requirement
        // hasn't changed, whether zero or some positive value.
        // This means that we can reuse the storage exactly
        // as-is and no allocation needs to occur. Therefore,
        // if we reuse a value for the same fixed-size type -
        // an int32 for example - then we can continue to reuse
        // the same storage for each value, avoiding memory
        // reallocation.
        return ptr;
    }
    if (old_size==0) {
        // In this case we need to allocate new storage space
        // where previously none was allocated. This means
        // that a full allocation is required.
        return BoltMem_allocate(new_size);
    }
    if (new_size==0) {
        // In this case, we are moving from previously having
        // data to no longer requiring any storage space. This
        // means that we can free up the previously-allocated
        // space.
        return BoltMem_deallocate(ptr, old_size);
    }
    // Finally, this case deals with previous allocation
    // and a new allocation requirement, but of different
    // sizes. Here, we `realloc`, which should be more
    // efficient than a naïve deallocation followed by a
    // brand new allocation.
    return BoltMem_reallocate(ptr, old_size, new_size);
}

void* BoltMem_duplicate(const void* ptr, int64_t ptr_size)
{
    if (ptr==NULL) {
        return NULL;
    }
    void* p = BoltMem_allocate(ptr_size);
    memcpy(p, ptr, ptr_size);
    return p;
}

int64_t BoltMem_current_allocation()
{
    return __allocation;
}

int64_t BoltMem_peak_allocation()
{
    return __peak_allocation;
}

int64_t BoltMem_allocation_events()
{
    return __allocation_events;
}
