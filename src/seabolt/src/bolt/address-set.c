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
#include "address-private.h"
#include "address-set-private.h"
#include "mem.h"

BoltAddressSet* BoltAddressSet_create()
{
    struct BoltAddressSet* set = (struct BoltAddressSet*) BoltMem_allocate(SIZE_OF_ADDRESS_SET);
    set->size = 0;
    set->elements = NULL;
    return set;
}

void BoltAddressSet_destroy(BoltAddressSet* set)
{
    for (int i = 0; i<set->size; i++) {
        BoltAddress_destroy((BoltAddress*) set->elements[i]);
    }
    BoltMem_deallocate(set->elements, set->size*sizeof(BoltAddress*));
    BoltMem_deallocate(set, SIZE_OF_ADDRESS_SET);
}

int32_t BoltAddressSet_size(BoltAddressSet* set)
{
    return set->size;
}

int32_t BoltAddressSet_index_of(BoltAddressSet* set, const BoltAddress* address)
{
    for (int i = 0; i<set->size; i++) {
        BoltAddress* current = (BoltAddress*) set->elements[i];

        if (strcmp(address->host, current->host)==0 && strcmp(address->port, current->port)==0) {
            return i;
        }
    }

    return -1;
}

int32_t BoltAddressSet_add(BoltAddressSet* set, const BoltAddress* address)
{
    if (BoltAddressSet_index_of(set, address)==-1) {
        set->elements = BoltMem_reallocate(set->elements, set->size*sizeof(BoltAddress*),
                (set->size+1)*sizeof(BoltAddress*));
        set->elements[set->size] = BoltAddress_create(address->host, address->port);
        set->size++;
        return set->size-1;
    }

    return -1;
}

int32_t BoltAddressSet_remove(BoltAddressSet* set, const BoltAddress* address)
{
    int index = BoltAddressSet_index_of(set, address);
    if (index>=0) {
        volatile BoltAddress** old_elements = set->elements;
        volatile BoltAddress** new_elements = BoltMem_allocate((set->size-1)*sizeof(BoltAddress*));
        int new_elements_index = 0;
        for (int i = 0; i<set->size; i++) {
            if (i==index) {
                continue;
            }

            new_elements[new_elements_index++] = old_elements[i];
        }

        BoltAddress_destroy((BoltAddress*) set->elements[index]);
        BoltMem_deallocate(old_elements, set->size*sizeof(BoltAddress*));
        set->elements = new_elements;
        set->size--;
        return index;
    }
    return -1;
}

void BoltAddressSet_replace(BoltAddressSet* dest, BoltAddressSet* source)
{
    for (int i = 0; i<dest->size; i++) {
        BoltAddress_destroy((BoltAddress*) dest->elements[i]);
    }

    dest->elements = BoltMem_reallocate(dest->elements, dest->size*sizeof(BoltAddress*),
            source->size*sizeof(BoltAddress*));
    for (int i = 0; i<source->size; i++) {
        dest->elements[i] = BoltAddress_create(source->elements[i]->host, source->elements[i]->port);
    }
    dest->size = source->size;
}

void BoltAddressSet_add_all(BoltAddressSet* destination, BoltAddressSet* source)
{
    for (int i = 0; i<source->size; i++) {
        BoltAddressSet_add(destination, (BoltAddress*) source->elements[i]);
    }
}
