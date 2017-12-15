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
#include <memory.h>
#include <warden.h>


struct BoltBuffer* BoltBuffer_create(size_t size)
{
    struct BoltBuffer* buffer = BoltMem_allocate(sizeof(struct BoltBuffer));
    buffer->size = size;
    buffer->data = BoltMem_allocate(buffer->size);
    buffer->extent = 0;
    buffer->cursor = 0;
    return buffer;
}

void BoltBuffer_destroy(struct BoltBuffer* buffer)
{
    buffer->data = BoltMem_deallocate(buffer->data, buffer->size);
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

void BoltBuffer_load_int8(struct BoltBuffer* buffer, int8_t x)
{
    char* target = BoltBuffer_load_target(buffer, sizeof(x));
    target[0] = (char)(x);
}

void BoltBuffer_load_uint8(struct BoltBuffer* buffer, uint8_t x)
{
    char* target = BoltBuffer_load_target(buffer, sizeof(x));
    target[0] = (char)(x);
}

void BoltBuffer_load_uint16_be(struct BoltBuffer* buffer, uint16_t x)
{
    char* target = BoltBuffer_load_target(buffer, sizeof(x));
    memcpy_be(&target[0], &x, sizeof(x));
}

void BoltBuffer_load_int16_be(struct BoltBuffer* buffer, int16_t x)
{
    char* target = BoltBuffer_load_target(buffer, sizeof(x));
    memcpy_be(&target[0], &x, sizeof(x));
}

void BoltBuffer_load_int32_be(struct BoltBuffer* buffer, int32_t x)
{
    char* target = BoltBuffer_load_target(buffer, sizeof(x));
    memcpy_be(&target[0], &x, sizeof(x));
}

void BoltBuffer_load_int64_be(struct BoltBuffer* buffer, int64_t x)
{
    char* target = BoltBuffer_load_target(buffer, sizeof(x));
    memcpy_be(&target[0], &x, sizeof(x));
}

void BoltBuffer_load_float_be(struct BoltBuffer* buffer, float x)
{
    char* target = BoltBuffer_load_target(buffer, (int)((sizeof(x))));
    memcpy_be(&target[0], &x, sizeof(x));
}

void BoltBuffer_load_double_be(struct BoltBuffer* buffer, double x)
{
    char* target = BoltBuffer_load_target(buffer, (int)((sizeof(x))));
    memcpy_be(&target[0], &x, sizeof(x));
}

int BoltBuffer_unloadable(struct BoltBuffer* buffer)
{
    return buffer->extent - buffer->cursor;
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

int BoltBuffer_unload_uint16_be(struct BoltBuffer* buffer, uint16_t* x)
{
    if (BoltBuffer_unloadable(buffer) < sizeof(*x)) return -1;
    memcpy_be(x, &buffer->data[buffer->cursor], sizeof(*x));
    buffer->cursor += sizeof(*x);
}

int BoltBuffer_unload_int8(struct BoltBuffer* buffer, int8_t* x)
{
    if (BoltBuffer_unloadable(buffer) < sizeof(*x)) return -1;
    memcpy_be(x, &buffer->data[buffer->cursor], sizeof(*x));
    buffer->cursor += sizeof(*x);
}

int BoltBuffer_unload_int16_be(struct BoltBuffer* buffer, int16_t* x)
{
    if (BoltBuffer_unloadable(buffer) < sizeof(*x)) return -1;
    memcpy_be(x, &buffer->data[buffer->cursor], sizeof(*x));
    buffer->cursor += sizeof(*x);
}

int BoltBuffer_unload_int32_be(struct BoltBuffer* buffer, int32_t* x)
{
    if (BoltBuffer_unloadable(buffer) < sizeof(*x)) return -1;
    memcpy_be(x, &buffer->data[buffer->cursor], sizeof(*x));
    buffer->cursor += sizeof(*x);
}

int BoltBuffer_unload_int64_be(struct BoltBuffer* buffer, int64_t* x)
{
    if (BoltBuffer_unloadable(buffer) < sizeof(*x)) return -1;
    memcpy_be(x, &buffer->data[buffer->cursor], sizeof(*x));
    buffer->cursor += sizeof(*x);
}

int BoltBuffer_unload_float_be(struct BoltBuffer* buffer, float* x)
{
    if (BoltBuffer_unloadable(buffer) < sizeof(*x)) return -1;
    memcpy_be(x, &buffer->data[buffer->cursor], sizeof(*x));
    buffer->cursor += sizeof(*x);
}

int BoltBuffer_unload_double_be(struct BoltBuffer* buffer, double* x)
{
    if (BoltBuffer_unloadable(buffer) < sizeof(*x)) return -1;
    memcpy_be(x, &buffer->data[buffer->cursor], sizeof(*x));
    buffer->cursor += sizeof(*x);
}
