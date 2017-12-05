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

#ifndef SEABOLT_BUFFER
#define SEABOLT_BUFFER


#include <stddef.h>
#include <string.h>
#include <stdint.h>

#include "mem.h"


typedef struct
{
    size_t size;
    size_t load_cursor;
    size_t unload_cursor;
    char* data;
} BoltBuffer;


BoltBuffer* BoltBuffer_create(size_t size);

void BoltBuffer_destroy(BoltBuffer* buffer);

/**
 * Return the amount of loadable space.
 *
 * @param buffer
 * @return
 */
size_t BoltBuffer_loadable(BoltBuffer* buffer);

size_t BoltBuffer_load_bytes(BoltBuffer* buffer, const char* data, size_t size);

char* BoltBuffer_load(BoltBuffer* buffer, size_t size);

size_t BoltBuffer_load_int32be(BoltBuffer* buffer, int32_t x);

size_t BoltBuffer_unloadable(BoltBuffer* buffer);

char* BoltBuffer_unload(BoltBuffer* buffer, size_t size);

int32_t BoltBuffer_unload_int32be(BoltBuffer* buffer);


#endif // SEABOLT_BUFFER
