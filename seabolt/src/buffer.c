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
#include "buffer.h"


BoltBuffer* BoltBuffer_create(size_t size)
{
    BoltBuffer* buffer = BoltMem_allocate(sizeof(BoltBuffer));
    buffer->size = size;
    buffer->data = BoltMem_allocate(buffer->size);
    buffer->load_cursor = 0;
    buffer->unload_cursor = 0;
    return buffer;
}

void BoltBuffer_destroy(BoltBuffer* buffer)
{
    buffer->data = BoltMem_deallocate(buffer->data, buffer->size);
    BoltMem_deallocate(buffer, sizeof(BoltBuffer));
}

size_t BoltBuffer_loadable(BoltBuffer* buffer)
{
    return buffer->size - buffer->load_cursor;
}

char* BoltBuffer_load(BoltBuffer* buffer, size_t size)
{
    size_t available = BoltBuffer_loadable(buffer);
    if (size > available) return NULL;
    size_t cursor = buffer->load_cursor;
    buffer->load_cursor += size;
    return &buffer->data[cursor];
}

size_t BoltBuffer_load_bytes(BoltBuffer* buffer, const char* data, size_t size)
{
    size_t writeable = BoltBuffer_loadable(buffer);
    if (size > writeable) size = writeable;
    memcpy(&buffer->data[buffer->load_cursor], data, size);
    buffer->load_cursor += size;
    return size;
}

size_t BoltBuffer_load_int32be(BoltBuffer* buffer, int32_t x)
{
    if (BoltBuffer_loadable(buffer) < 4) return 0;
    buffer->data[buffer->load_cursor++] = (char)(x >> 24);
    buffer->data[buffer->load_cursor++] = (char)(x >> 16);
    buffer->data[buffer->load_cursor++] = (char)(x >> 8);
    buffer->data[buffer->load_cursor++] = (char)(x);
    return 4;
}

size_t BoltBuffer_unloadable(BoltBuffer* buffer)
{
    return buffer->load_cursor - buffer->unload_cursor;
}

char* BoltBuffer_unload(BoltBuffer* buffer, size_t size)
{
    size_t available = BoltBuffer_unloadable(buffer);
    if (size > available) return NULL;
    size_t cursor = buffer->unload_cursor;
    buffer->unload_cursor += size;
    if (buffer->unload_cursor == buffer->load_cursor)
    {
        buffer->load_cursor = 0;
        buffer->unload_cursor = 0;
    }
    return &buffer->data[cursor];
}

int32_t BoltBuffer_unload_int32be(BoltBuffer* buffer)
{
    if (BoltBuffer_unloadable(buffer) < 4) return 0;
    return (buffer->data[buffer->unload_cursor++] << 24) |
           (buffer->data[buffer->unload_cursor++] << 16) |
           (buffer->data[buffer->unload_cursor++] << 8) |
           (buffer->data[buffer->unload_cursor++]);
}
