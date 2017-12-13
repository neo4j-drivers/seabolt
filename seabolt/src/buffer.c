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


#include <buffer.h>
#include <limits.h>
#include <bolt.h>
#include <memory.h>


struct BoltBuffer* BoltBuffer_create(size_t size)
{
    struct BoltBuffer* buffer = BoltMem_allocate(sizeof(struct BoltBuffer));
    buffer->size = size;
    buffer->data = BoltMem_allocate(buffer->size);
    buffer->extent = 0;
    buffer->cursor = 0;
    buffer->num_stops = 0;
    buffer->stops = BoltMem_allocate(0);
    return buffer;
}

void BoltBuffer_destroy(struct BoltBuffer* buffer)
{
    buffer->data = BoltMem_deallocate(buffer->data, buffer->size);
    buffer->stops = BoltMem_deallocate(buffer->stops, buffer->num_stops * sizeof(int));
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
    size_t available = buffer->size - buffer->extent;
    return available > INT_MAX ? INT_MAX : (int)(available);
}

char* BoltBuffer_load_target(struct BoltBuffer* buffer, int size)
{
    int available = BoltBuffer_loadable(buffer);
    if (size > available)
    {
        size_t new_size = buffer->size + (size - available);
        buffer->data = BoltMem_reallocate(buffer->data, buffer->size, new_size);
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

void BoltBuffer_load_uint8(struct BoltBuffer* buffer, uint8_t x)
{
    char* target = BoltBuffer_load_target(buffer, sizeof(x));
    target[0] = (char)(x);
}

void BoltBuffer_load_int32be(struct BoltBuffer* buffer, int32_t x)
{
    char* target = BoltBuffer_load_target(buffer, sizeof(x));
    target[0] = (char)(x >> 24);
    target[1] = (char)(x >> 16);
    target[2] = (char)(x >> 8);
    target[3] = (char)(x);
}

void BoltBuffer_push_stop(struct BoltBuffer* buffer)
{
    int num_stops = buffer->num_stops + 1;
    buffer->stops = BoltMem_reallocate(buffer->stops, buffer->num_stops * sizeof(int), num_stops * sizeof(int));
    buffer->stops[buffer->num_stops] = buffer->extent;
    buffer->num_stops = num_stops;
}

int BoltBuffer_next_stop(struct BoltBuffer* buffer)
{
    return buffer->num_stops == 0 ? -1 : buffer->stops[0];
}

void BoltBuffer_pull_stop(struct BoltBuffer* buffer)
{
    if (buffer->num_stops > 0)
    {
        int num_stops = buffer->num_stops - 1;
        memcpy(&buffer->stops[0], &buffer->stops[1], num_stops * sizeof(int));
        buffer->stops = BoltMem_reallocate(buffer->stops, buffer->num_stops * sizeof(int), num_stops * sizeof(int));
        buffer->num_stops = num_stops;
    }
}

int BoltBuffer_unloadable(struct BoltBuffer* buffer)
{
    int stop = BoltBuffer_next_stop(buffer);
    return (stop == -1 ? buffer->extent : stop) - buffer->cursor;
}

char* BoltBuffer_unload_target(struct BoltBuffer* buffer, int size)
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

int BoltBuffer_peek_uint8(struct BoltBuffer* buffer, uint8_t* x)
{
    if (BoltBuffer_unloadable(buffer) < 1) return -1;
    *x = (uint8_t)(buffer->data[buffer->cursor]);
}

int BoltBuffer_unload_uint8(struct BoltBuffer* buffer, uint8_t* x)
{
    if (BoltBuffer_unloadable(buffer) < 1) return -1;
    *x = (uint8_t)(buffer->data[buffer->cursor++]);
}

int BoltBuffer_unload_uint16be(struct BoltBuffer* buffer, uint16_t* x)
{
    if (BoltBuffer_unloadable(buffer) < 2) return -1;
    *x = (
            ((uint16_t)(uint8_t)(buffer->data[buffer->cursor++]) << 8) |
            ((uint16_t)(uint8_t)(buffer->data[buffer->cursor++]))
    );
}

int BoltBuffer_unload_int8(struct BoltBuffer* buffer, int8_t* x)
{
    if (BoltBuffer_unloadable(buffer) < 1) return -1;
    *x = (int8_t)(uint8_t)(buffer->data[buffer->cursor++]);
}

int BoltBuffer_unload_int16be(struct BoltBuffer* buffer, int16_t* x)
{
    if (BoltBuffer_unloadable(buffer) < 2) return -1;
    *x = (int16_t)(
            ((uint16_t)(uint8_t)(buffer->data[buffer->cursor++]) << 8) |
            ((uint16_t)(uint8_t)(buffer->data[buffer->cursor++]))
    );
}

int BoltBuffer_unload_int32be(struct BoltBuffer* buffer, int32_t* x)
{
    if (BoltBuffer_unloadable(buffer) < 4) return -1;
    *x = (int32_t)(
            ((uint32_t)(uint8_t)(buffer->data[buffer->cursor++]) << 24) |
            ((uint32_t)(uint8_t)(buffer->data[buffer->cursor++]) << 16) |
            ((uint32_t)(uint8_t)(buffer->data[buffer->cursor++]) << 8) |
            ((uint32_t)(uint8_t)(buffer->data[buffer->cursor++]))
    );
}

int BoltBuffer_unload_int64be(struct BoltBuffer* buffer, int64_t* x)
{
    if (BoltBuffer_unloadable(buffer) < 8) return -1;
    *x = (int64_t)(
            ((uint64_t)(uint8_t)(buffer->data[buffer->cursor++]) << 56) |
            ((uint64_t)(uint8_t)(buffer->data[buffer->cursor++]) << 48) |
            ((uint64_t)(uint8_t)(buffer->data[buffer->cursor++]) << 40) |
            ((uint64_t)(uint8_t)(buffer->data[buffer->cursor++]) << 32) |
            ((uint64_t)(uint8_t)(buffer->data[buffer->cursor++]) << 24) |
            ((uint64_t)(uint8_t)(buffer->data[buffer->cursor++]) << 16) |
            ((uint64_t)(uint8_t)(buffer->data[buffer->cursor++]) << 8) |
            ((uint64_t)(uint8_t)(buffer->data[buffer->cursor++]))
    );
}
