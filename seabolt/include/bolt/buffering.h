/*
 * Copyright (c) 2002-2018 "Neo Technology,"
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

#ifndef SEABOLT_BUFFERING
#define SEABOLT_BUFFERING


#include "config.h"

#include <stddef.h>
#include <stdint.h>


/**
 * General purpose data buffer.
 */
struct BoltBuffer
{
    int size;
    int extent;
    int cursor;
    char * data;
};


/**
 * Create a buffer.
 *
 * @param size
 * @return
 */
struct BoltBuffer * BoltBuffer_create(int size);

/**
 * Destroy a buffer.
 *
 * @param buffer
 */
void BoltBuffer_destroy(struct BoltBuffer * buffer);

/**
 * Compact a buffer by removing unused space.
 *
 * @param buffer
 */
void BoltBuffer_compact(struct BoltBuffer * buffer);

/**
 * Return the amount of loadable space in a buffer, in bytes.
 *
 * @param buffer
 * @return
 */
int BoltBuffer_loadable(struct BoltBuffer * buffer);

/**
 * Allocate space in a buffer for loading data and return a pointer to that space.
 *
 * @param buffer
 * @param size
 * @return
 */
char * BoltBuffer_load_pointer(struct BoltBuffer * buffer, int size);

/**
 * Load data into a buffer.
 *
 * @param buffer
 * @param data
 * @param size
 */
void BoltBuffer_load(struct BoltBuffer * buffer, const char * data, int size);

/**
 * Load an unsigned 8-bit integer into a buffer.
 *
 * @param buffer
 * @param x
 */
void BoltBuffer_load_u8(struct BoltBuffer * buffer, uint8_t x);

/**
 * Load an unsigned 16-bit integer (big-endian) into a buffer.
 *
 * @param buffer
 * @param x
 */
void BoltBuffer_load_u16be(struct BoltBuffer * buffer, uint16_t x);

/**
 * Load a signed 8-bit integer into a buffer.
 *
 * @param buffer
 * @param x
 */
void BoltBuffer_load_i8(struct BoltBuffer * buffer, int8_t x);

/**
 * Load a signed 16-bit integer (big-endian) into a buffer.
 *
 * @param buffer
 * @param x
 */
void BoltBuffer_load_i16be(struct BoltBuffer * buffer, int16_t x);

/**
 * Load a signed 32-bit integer (big-endian) into a buffer.
 *
 * @param buffer
 * @param x
 */
void BoltBuffer_load_i32be(struct BoltBuffer * buffer, int32_t x);

/**
 * Load a signed 64-bit integer (big-endian) into a buffer.
 *
 * @param buffer
 * @param x
 */
void BoltBuffer_load_i64be(struct BoltBuffer * buffer, int64_t x);

/**
 * Load a double precision floating point number (big-endian) into a buffer.
 *
 * @param buffer
 * @param x
 */
void BoltBuffer_load_f64be(struct BoltBuffer * buffer, double x);

/**
 * Return the amount of unloadable data in a buffer, in bytes.
 *
 * @param buffer
 * @return
 */
int BoltBuffer_unloadable(struct BoltBuffer * buffer);

/**
 * Mark data in a buffer for unloading and return a pointer to that data.
 *
 * @param buffer
 * @param size
 * @return
 */
char * BoltBuffer_unload_pointer(struct BoltBuffer * buffer, int size);

/**
 * Unload data from a buffer.
 *
 * @param buffer
 * @param data
 * @param size
 * @return
 */
int BoltBuffer_unload(struct BoltBuffer * buffer, char * data, int size);

/**
 * Return the next unloadable byte in a buffer as an unsigned 8-bit integer.
 *
 * @param buffer
 * @param x
 * @return
 */
int BoltBuffer_peek_u8(struct BoltBuffer * buffer, uint8_t * x);

/**
 * Unload an unsigned 8-bit integer from a buffer.
 *
 * @param buffer
 * @param x
 * @return
 */
int BoltBuffer_unload_u8(struct BoltBuffer * buffer, uint8_t * x);


/**
 * Unload an unsigned 16-bit integer (big endian) from a buffer.
 *
 * @param buffer
 * @param x
 * @return
 */
int BoltBuffer_unload_u16be(struct BoltBuffer * buffer, uint16_t * x);

/**
 * Unload a signed 8-bit integer from a buffer.
 *
 * @param buffer
 * @param x
 * @return
 */
int BoltBuffer_unload_i8(struct BoltBuffer * buffer, int8_t * x);

/**
 * Unload a signed 16-bit integer (big endian) from a buffer.
 *
 * @param buffer
 * @param x
 * @return
 */
int BoltBuffer_unload_i16be(struct BoltBuffer * buffer, int16_t * x);

/**
 * Unload a signed 32-bit integer (big endian) from a buffer.
 *
 * @param buffer
 * @param x
 * @return
 */
int BoltBuffer_unload_i32be(struct BoltBuffer * buffer, int32_t * x);

/**
 * Unload a signed 64-bit integer (big endian) from a buffer.
 *
 * @param buffer
 * @param x
 * @return
 */
int BoltBuffer_unload_i64be(struct BoltBuffer * buffer, int64_t * x);

/**
 * Unload a double precision floating point number (big endian) from a buffer.
 *
 * @param buffer
 * @param x
 * @return
 */
int BoltBuffer_unload_f64be(struct BoltBuffer * buffer, double * x);


#endif // SEABOLT_BUFFERING
