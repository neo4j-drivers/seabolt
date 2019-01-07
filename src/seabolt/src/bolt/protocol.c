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
#include "mem.h"
#include "protocol.h"
#include "values-private.h"

#define BOLT_MAX_CHUNK_SIZE 65535

#define TRY(code) { int status_try = (code); if (status_try != BOLT_SUCCESS) { return status_try; } }

struct BoltMessage* BoltMessage_create(int8_t code, int32_t n_fields)
{
    const size_t size = sizeof(struct BoltMessage);
    struct BoltMessage* message = BoltMem_allocate(size);
    message->code = code;
    message->fields = BoltValue_create();
    BoltValue_format_as_List(message->fields, n_fields);
    return message;
}

void BoltMessage_destroy(struct BoltMessage* message)
{
    BoltValue_destroy(message->fields);
    BoltMem_deallocate(message, sizeof(struct BoltMessage));
}

struct BoltValue* BoltMessage_param(struct BoltMessage* message, int32_t index)
{
    if (index>=message->fields->size) {
        return NULL;
    }

    return BoltList_value(message->fields, index);
}

int write_message(struct BoltMessage* message, check_struct_signature_func check_writable_struct,
        struct BoltBuffer* buffer, const struct BoltLog* log)
{
    if (check_writable_struct(message->code)) {
        TRY(load_structure_header(buffer, message->code, (int8_t) (message->fields->size)));
        for (int32_t i = 0; i<message->fields->size; i++) {
            TRY(load(check_writable_struct, buffer, BoltList_value(message->fields, i), log));
        }
        return BOLT_SUCCESS;
    }
    return BOLT_PROTOCOL_UNSUPPORTED_TYPE;
}

void push_to_transmission(struct BoltBuffer* msg_buffer, struct BoltBuffer* tx_buffer)
{
    // loop through data, generate several chunks if it's larger than max chunk size
    int total_size = BoltBuffer_unloadable(msg_buffer);
    int total_remaining = total_size;
    char header[2];
    while (total_remaining>0) {
        int current_size = total_remaining>BOLT_MAX_CHUNK_SIZE ? BOLT_MAX_CHUNK_SIZE : total_remaining;
        header[0] = (char) (current_size >> 8);
        header[1] = (char) (current_size);
        BoltBuffer_load(tx_buffer, &header[0], sizeof(header));
        BoltBuffer_load(tx_buffer, BoltBuffer_unload_pointer(msg_buffer, current_size), current_size);
        total_remaining -= current_size;
    }

    header[0] = (char) (0);
    header[1] = (char) (0);
    BoltBuffer_load(tx_buffer, &header[0], sizeof(header));
    BoltBuffer_compact(msg_buffer);
}

