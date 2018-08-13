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


#ifndef SEABOLT_LOGGING
#define SEABOLT_LOGGING


#include <stdio.h>

#include "config.h"
#include "connections.h"
#include "values.h"


void BoltLog_set_file(FILE * log_file);

void BoltLog_info(const char* message, ...);

void BoltLog_error(const char* message, ...);

void BoltLog_value(struct BoltValue * value, int32_t protocol_version, const char * prefix, const char * suffix);

void BoltLog_message(const char * peer, bolt_request_t request_id, int16_t code, struct BoltValue * fields, int32_t protocol_version);


#endif // SEABOLT_LOGGING
