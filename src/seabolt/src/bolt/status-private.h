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
#ifndef SEABOLT_STATUS_PRIVATE_H
#define SEABOLT_STATUS_PRIVATE_H

#include "status.h"

struct BoltStatus {
    BoltConnectionState state;
    int32_t error;
    char* error_ctx;
    uint64_t error_ctx_size;
};

BoltStatus* BoltStatus_create_with_ctx(uint64_t ctx_size);

void BoltStatus_set_error(BoltStatus* status, int error);

void BoltStatus_set_error_with_ctx(BoltStatus* status, int error, const char* error_ctx_format, ...);

#endif //SEABOLT_STATUS_PRIVATE_H
