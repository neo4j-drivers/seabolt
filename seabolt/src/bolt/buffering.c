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


#include <assert.h>
#include <limits.h>
#include <memory.h>

#include "bolt/buffering.h"
#include "bolt/mem.h"


static const char REPLACEMENT_CHARACTER[2] = {(char)(0xFF), (char)(0xFD)};


struct BoltBuffer* BoltBuffer_create(int size)
{
    struct BoltBuffer* buffer = BoltMem_allocate(sizeof(struct BoltBuffer));
    buffer->size = size;
    buffer->data = BoltMem_allocate((size_t)(buffer->size));
    buffer->extent = 0;
    buffer->cursor = 0;
    return buffer;
}

void BoltBuffer_destroy(struct BoltBuffer* buffer)
{
    buffer->data = BoltMem_deallocate(buffer->data, (size_t)(buffer->size));
    BoltMem_deallocate(buffer, sizeof(struct BoltBuffer));
}

void BoltBuffer_compact(struct BoltBuffer* buffer)
{
    if (buffer->cursor > 0)
    {
        int available = buffer->extent - buffer->cursor;
        if (available > 0)
        {
            memcpy(&buffer->data[0], &buffer->data[buffer->cursor], (size_t)(available));
        }
        buffer->cursor = 0;
        buffer->extent = available;
    }
}

int BoltBuffer_loadable(struct BoltBuffer* buffer)
{
    int available = buffer->size - buffer->extent;
    return available > INT_MAX ? INT_MAX : available;
}

char* BoltBuffer_load_target(struct BoltBuffer* buffer, int size)
{
    int available = BoltBuffer_loadable(buffer);
    if (size > available)
    {
        int new_size = buffer->size + (size - available);
        buffer->data = BoltMem_reallocate(buffer->data, (size_t)(buffer->size), (size_t)(new_size));
        buffer->size = new_size;
    }
    int extent = buffer->extent;
    buffer->extent += size;
    return &buffer->data[extent];
}

void BoltBuffer_load(struct BoltBuffer* buffer, const char* data, int size)
{
    char* target = BoltBuffer_load_target(buffer, size);
    memcpy(target, data, size >= 0 ? (size_t)(size) : 0);
}

int BoltBuffer_sizeof_utf8_code_point(const uint32_t code_point)
{
    if (code_point < 0x80)
    {
        return 1;
    }
    if (code_point < 0x800)
    {
        return 2;
    }
    if (code_point < 0x10000)
    {
        return 3;
    }
    if (code_point < 0x110000)
    {
        return 4;
    }
    return sizeof(REPLACEMENT_CHARACTER);
}

void BoltBuffer_load_utf8_code_point(struct BoltBuffer * buffer, const uint32_t code_point)
{
    if (code_point < 0x80)
    {
        char bytes = (char)(code_point);
        BoltBuffer_load(buffer, &bytes, 1);
    }
    else if (code_point < 0x800)
    {
        char bytes[2] = {
                (char)(((code_point >> 6) & 0b00011111) | 0b11000000),
                (char)(((code_point >> 0) & 0b00111111) | 0b10000000),
        };
        BoltBuffer_load(buffer, &bytes[0], 2);
    }
    else if (code_point < 0x10000)
    {
        char bytes[3] = {
                (char)(((code_point >> 12) & 0b00001111) | 0b11100000),
                (char)(((code_point >> 6) & 0b00111111) | 0b10000000),
                (char)(((code_point >> 0) & 0b00111111) | 0b10000000),
        };
        BoltBuffer_load(buffer, &bytes[0], 3);
    }
    else if (code_point < 0x110000)
    {
        char bytes[4] = {
                (char)(((code_point >> 18) & 0b00000111) | 0b11110000),
                (char)(((code_point >> 12) & 0b00111111) | 0b10000000),
                (char)(((code_point >> 6) & 0b00111111) | 0b10000000),
                (char)(((code_point >> 0) & 0b00111111) | 0b10000000),
        };
        BoltBuffer_load(buffer, &bytes[0], 4);
    }
    else
    {
        BoltBuffer_load(buffer, &REPLACEMENT_CHARACTER[0], sizeof(REPLACEMENT_CHARACTER));
    }
}

void BoltBuffer_load_i8(struct BoltBuffer * buffer, int8_t x)
{
    char* target = BoltBuffer_load_target(buffer, sizeof(x));
    target[0] = (char)(x);
}

void BoltBuffer_load_u8(struct BoltBuffer * buffer, uint8_t x)
{
    char* target = BoltBuffer_load_target(buffer, sizeof(x));
    target[0] = (char)(x);
}

void BoltBuffer_load_u16be(struct BoltBuffer * buffer, uint16_t x)
{
    char* target = BoltBuffer_load_target(buffer, sizeof(x));
    memcpy_be(&target[0], &x, sizeof(x));
}

void BoltBuffer_load_i16be(struct BoltBuffer * buffer, int16_t x)
{
    char* target = BoltBuffer_load_target(buffer, sizeof(x));
    memcpy_be(&target[0], &x, sizeof(x));
}

void BoltBuffer_load_i32be(struct BoltBuffer * buffer, int32_t x)
{
    char* target = BoltBuffer_load_target(buffer, sizeof(x));
    memcpy_be(&target[0], &x, sizeof(x));
}

void BoltBuffer_load_i64be(struct BoltBuffer * buffer, int64_t x)
{
    char* target = BoltBuffer_load_target(buffer, sizeof(x));
    memcpy_be(&target[0], &x, sizeof(x));
}

void BoltBuffer_load_f64be(struct BoltBuffer * buffer, double x)
{
    char* target = BoltBuffer_load_target(buffer, (int)((sizeof(x))));
    memcpy_be(&target[0], &x, sizeof(x));
}

int BoltBuffer_unloadable(struct BoltBuffer * buffer)
{
    return buffer->extent - buffer->cursor;
}

char * BoltBuffer_unload_target(struct BoltBuffer* buffer, int size)
{
    int available = BoltBuffer_unloadable(buffer);
    if (size > available) return NULL;
    int cursor = buffer->cursor;
    buffer->cursor += size;
    if (buffer->cursor == buffer->extent)
    {
        buffer->extent = 0;
        buffer->cursor = 0;
    }
    return &buffer->data[cursor];
}

int BoltBuffer_unload(struct BoltBuffer* buffer, char* data, int size)
{
    int available = BoltBuffer_unloadable(buffer);
    if (size > available) return -1;
    int cursor = buffer->cursor;
    buffer->cursor += size;
    if (buffer->cursor == buffer->extent)
    {
        BoltBuffer_compact(buffer);
    }
    memcpy(data, &buffer->data[cursor], (size_t)(size));
    return size;
}

int BoltBuffer_peek_u8(struct BoltBuffer * buffer, uint8_t * x)
{
    if (BoltBuffer_unloadable(buffer) < 1) return -1;
    *x = (uint8_t)(buffer->data[buffer->cursor]);
    return 0;
}

int BoltBuffer_unload_u8(struct BoltBuffer * buffer, uint8_t * x)
{
    if (BoltBuffer_unloadable(buffer) < 1) return -1;
    *x = (uint8_t)(buffer->data[buffer->cursor++]);
    return 0;
}

int BoltBuffer_unload_u16be(struct BoltBuffer * buffer, uint16_t * x)
{
    if (BoltBuffer_unloadable(buffer) < sizeof(*x)) return -1;
    memcpy_be(x, &buffer->data[buffer->cursor], sizeof(*x));
    buffer->cursor += sizeof(*x);
    return 0;
}

int BoltBuffer_unload_i8(struct BoltBuffer * buffer, int8_t * x)
{
    if (BoltBuffer_unloadable(buffer) < sizeof(*x)) return -1;
    memcpy_be(x, &buffer->data[buffer->cursor], sizeof(*x));
    buffer->cursor += sizeof(*x);
    return 0;
}

int BoltBuffer_unload_i16be(struct BoltBuffer * buffer, int16_t * x)
{
    if (BoltBuffer_unloadable(buffer) < sizeof(*x)) return -1;
    memcpy_be(x, &buffer->data[buffer->cursor], sizeof(*x));
    buffer->cursor += sizeof(*x);
    return 0;
}

int BoltBuffer_unload_i32be(struct BoltBuffer * buffer, int32_t * x)
{
    if (BoltBuffer_unloadable(buffer) < sizeof(*x)) return -1;
    memcpy_be(x, &buffer->data[buffer->cursor], sizeof(*x));
    buffer->cursor += sizeof(*x);
    return 0;
}

int BoltBuffer_unload_i64be(struct BoltBuffer * buffer, int64_t * x)
{
    if (BoltBuffer_unloadable(buffer) < sizeof(*x)) return -1;
    memcpy_be(x, &buffer->data[buffer->cursor], sizeof(*x));
    buffer->cursor += sizeof(*x);
    return 0;
}

int BoltBuffer_unload_f64be(struct BoltBuffer * buffer, double * x)
{
    if (BoltBuffer_unloadable(buffer) < sizeof(*x)) return -1;
    memcpy_be(x, &buffer->data[buffer->cursor], sizeof(*x));
    buffer->cursor += sizeof(*x);
    return 0;
}
