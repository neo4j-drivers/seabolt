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
#ifndef SEABOLT_ALL_PACKSTREAM_H
#define SEABOLT_ALL_PACKSTREAM_H

#include <stdint.h>

#include "buffering.h"
#include "log.h"
#include "values.h"

enum PackStreamType {
    PACKSTREAM_NULL,
    PACKSTREAM_BOOLEAN,
    PACKSTREAM_INTEGER,
    PACKSTREAM_FLOAT,
    PACKSTREAM_STRING,
    PACKSTREAM_BYTES,
    PACKSTREAM_LIST,
    PACKSTREAM_MAP,
    PACKSTREAM_STRUCTURE,
    PACKSTREAM_RESERVED,
};

typedef int (* check_struct_signature_func)(int16_t);

enum PackStreamType marker_type(uint8_t marker);

int load_structure_header(struct BoltBuffer* buffer, int16_t code, int8_t size);

int load(check_struct_signature_func check_struct_type, struct BoltBuffer* buffer, struct BoltValue* value,
        const struct BoltLog* log);

int unload(check_struct_signature_func check_struct_type, struct BoltBuffer* buffer, struct BoltValue* value,
        const struct BoltLog* log);

#endif //SEABOLT_ALL_PACKSTREAM_H
