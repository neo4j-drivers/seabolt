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
#include <stdint.h>


struct BoltBuffer
{
    size_t size;
    int extent;
    int cursor;
    char* data;
};


struct BoltBuffer* BoltBuffer_create(size_t size);

void BoltBuffer_destroy(struct BoltBuffer* buffer);

void BoltBuffer_compact(struct BoltBuffer* buffer);

int BoltBuffer_loadable(struct BoltBuffer* buffer);

char* BoltBuffer_load_target(struct BoltBuffer* buffer, int size);

void BoltBuffer_load(struct BoltBuffer* buffer, const char* data, int size);

void BoltBuffer_load_uint8(struct BoltBuffer* buffer, uint8_t x);

void BoltBuffer_load_uint16_be(struct BoltBuffer* buffer, uint16_t x);

void BoltBuffer_load_int8(struct BoltBuffer* buffer, int8_t x);

void BoltBuffer_load_int16_be(struct BoltBuffer* buffer, int16_t x);

void BoltBuffer_load_int32_be(struct BoltBuffer* buffer, int32_t x);

void BoltBuffer_load_int64_be(struct BoltBuffer* buffer, int64_t x);

void BoltBuffer_load_float_be(struct BoltBuffer* buffer, float x);

void BoltBuffer_load_double_be(struct BoltBuffer* buffer, double x);

int BoltBuffer_unloadable(struct BoltBuffer* buffer);

char* BoltBuffer_unload_target(struct BoltBuffer* buffer, int size);

int BoltBuffer_unload(struct BoltBuffer* buffer, char* data, int size);

int BoltBuffer_peek_uint8(struct BoltBuffer* buffer, uint8_t* x);

int BoltBuffer_unload_uint8(struct BoltBuffer* buffer, uint8_t* x);

int BoltBuffer_unload_uint16_be(struct BoltBuffer* buffer, uint16_t* x);

int BoltBuffer_unload_int8(struct BoltBuffer* buffer, int8_t* x);

int BoltBuffer_unload_int16_be(struct BoltBuffer* buffer, int16_t* x);

int BoltBuffer_unload_int32_be(struct BoltBuffer* buffer, int32_t* x);

int BoltBuffer_unload_int64_be(struct BoltBuffer* buffer, int64_t* x);

int BoltBuffer_unload_float_be(struct BoltBuffer* buffer, float* x);

int BoltBuffer_unload_double_be(struct BoltBuffer* buffer, double* x);


#endif // SEABOLT_BUFFER
