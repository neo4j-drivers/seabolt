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

typedef void (* log_func)(int state, const char* message);

struct BoltLog {
    int state;

    int error_enabled;
    int warning_enabled;
    int info_enabled;
    int debug_enabled;

    log_func error_logger;
    log_func warning_logger;
    log_func info_logger;
    log_func debug_logger;
};

SEABOLT_EXPORT struct BoltLog* BoltLog_create();

SEABOLT_EXPORT void BoltLog_destroy(struct BoltLog* log);

void BoltLog_error(const struct BoltLog* log, const char* format, ...);

void BoltLog_warning(const struct BoltLog* log, const char* format, ...);

void BoltLog_info(const struct BoltLog* log, const char* format, ...);

void BoltLog_debug(const struct BoltLog* log, const char* format, ...);

void
BoltLog_value(const struct BoltLog* log, const char* format, struct BoltValue* value,
        name_resolver_func struct_name_resolver);

void BoltLog_message(const struct BoltLog* log, const char* peer, bolt_request request_id, int16_t code,
        struct BoltValue* fields, name_resolver_func struct_name_resolver, name_resolver_func message_name_resolver);

#endif // SEABOLT_LOGGING
