/*
 * Copyright (c) 2002-2018 "Neo4j,"
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
#include "string-builder.h"

struct StringBuilder* StringBuilder_create()
{
    struct StringBuilder* builder = (struct StringBuilder*) malloc(sizeof(struct StringBuilder));
    builder->buffer = (char*) malloc(256*sizeof(char));
    builder->buffer[0] = 0;
    builder->buffer_pos = 0;
    builder->buffer_size = 256;
    return builder;
}

void StringBuilder_destroy(struct StringBuilder* builder)
{
    free(builder->buffer);
    free(builder);
}

void StringBuilder_ensure_buffer(struct StringBuilder* builder, int size_to_add)
{
    if (builder->buffer_size-builder->buffer_pos>size_to_add) {
        return;
    }

    int new_size = builder->buffer_pos+size_to_add;
    builder->buffer = (char*) realloc(builder->buffer, new_size);
    builder->buffer_size = new_size;
}

void StringBuilder_append(struct StringBuilder* builder, const char* string)
{
    StringBuilder_append_n(builder, string, (int)strlen(string));
}

void StringBuilder_append_n(struct StringBuilder* builder, const char* string, const int len)
{
    StringBuilder_ensure_buffer(builder, len+1);
    strncpy(builder->buffer+builder->buffer_pos, string, len);
    builder->buffer_pos += len;
    builder->buffer[builder->buffer_pos] = 0;
}

void StringBuilder_append_f(struct StringBuilder* builder, const char* format, ...)
{
    size_t size = 10240*sizeof(char);
    char* message_fmt = (char*) malloc(size);
    while (1) {
        va_list args;
        va_start(args, format);
        size_t written = vsnprintf(message_fmt, size, format, args);
        va_end(args);
        if (written<size) {
            break;
        }

        message_fmt = (char*) realloc(message_fmt, written+1);
        size = written+1;
    }

    StringBuilder_append(builder, message_fmt);

    free(message_fmt);
}

char* StringBuilder_get_string(struct StringBuilder* builder)
{
    return builder->buffer;
}

int StringBuilder_get_length(struct StringBuilder* builder)
{
    return builder->buffer_pos;
}
