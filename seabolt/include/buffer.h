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

/**
 * @file
 */

#ifndef SEABOLT_BUFFER
#define SEABOLT_BUFFER


#include <stddef.h>
#include <string.h>
#include <stdint.h>

#include "mem.h"


struct BoltBuffer
{
    size_t size;
    int extent;
    int cursor;
    char* data;
    int* stops;
    int num_stops;
};


struct BoltBuffer* BoltBuffer_create(size_t size);

void BoltBuffer_destroy(struct BoltBuffer* buffer);

int BoltBuffer_loadable(struct BoltBuffer* buffer);

char* BoltBuffer_loadTarget(struct BoltBuffer* buffer, int size);

void BoltBuffer_load(struct BoltBuffer* buffer, const char* data, int size);

void BoltBuffer_load_uint8(struct BoltBuffer* buffer, uint8_t x);

void BoltBuffer_load_int32be(struct BoltBuffer* buffer, int32_t x);

void BoltBuffer_pushStop(struct BoltBuffer* buffer);

int BoltBuffer_nextStop(struct BoltBuffer* buffer);

void BoltBuffer_pullStop(struct BoltBuffer* buffer);

int BoltBuffer_unloadable(struct BoltBuffer* buffer);

char* BoltBuffer_unloadTarget(struct BoltBuffer* buffer, int size);

int BoltBuffer_unload(struct BoltBuffer* buffer, char* data, int size);

int BoltBuffer_peek_uint8(struct BoltBuffer* buffer, uint8_t* x);

int BoltBuffer_unload_uint8(struct BoltBuffer* buffer, uint8_t* x);

int BoltBuffer_unload_uint16be(struct BoltBuffer* buffer, uint16_t* x);

int BoltBuffer_unload_int8(struct BoltBuffer* buffer, int8_t* x);

int BoltBuffer_unload_int16be(struct BoltBuffer* buffer, int16_t* x);

int BoltBuffer_unload_int32be(struct BoltBuffer* buffer, int32_t* x);

int BoltBuffer_unload_int64be(struct BoltBuffer* buffer, int64_t* x);


#endif // SEABOLT_BUFFER
