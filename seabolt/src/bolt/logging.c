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


#include <stdarg.h>
#include <stdlib.h>

#include "bolt/logging.h"
#include "bolt/mem.h"

#include "protocol/v1.h"

static FILE* __bolt_log_file;

void _perform_log_call(log_func func, const char* message, ...)
{
    int previous_size = 512*sizeof(char);
    int current_size = previous_size;
    char* fmt_message = BoltMem_allocate(current_size);
    while (1) {
        va_list args;
        va_start(args, message);
        int written = snprintf(fmt_message, current_size, message, args);
        va_end(args);
        if (written<=current_size) {
            break;
        }
        previous_size = current_size;
        current_size = current_size*2;
        fmt_message = BoltMem_reallocate(fmt_message, previous_size, current_size);
    }

    func(fmt_message);

    BoltMem_deallocate(fmt_message, current_size);
}

void BoltLog_error(const struct BoltLog* log, const char* message, ...)
{
    if (log!=NULL && log->error_enabled) {
        va_list args;
        va_start(args, message);
        _perform_log_call(log->error_logger, message, args);
        va_end(args);
    }
}

void BoltLog_warning(const struct BoltLog* log, const char* message, ...)
{
    if (log!=NULL && log->warning_enabled) {
        va_list args;
        va_start(args, message);
        _perform_log_call(log->warning_logger, message, args);
        va_end(args);
    }
}

void BoltLog_info(const struct BoltLog* log, const char* message, ...)
{
    if (log!=NULL && log->info_enabled) {
        va_list args;
        va_start(args, message);
        _perform_log_call(log->info_logger, message, args);
        va_end(args);

    }
}

void BoltLog_debug(const struct BoltLog* log, const char* message, ...)
{
    if (log!=NULL && log->debug_enabled) {
        va_list args;
        va_start(args, message);
        _perform_log_call(log->debug_logger, message, args);
        va_end(args);
    }
}

void BoltLog_value(struct BoltValue* value, int32_t protocol_version, const char* prefix, const char* suffix)
{
    if (__bolt_log_file==NULL) return;
    fprintf(__bolt_log_file, "bolt: %s", prefix);
    BoltValue_write(value, __bolt_log_file, protocol_version);
    fprintf(__bolt_log_file, "%s\n", suffix);
}

void BoltLog_message(const char* peer, bolt_request_t request_id, int16_t code, struct BoltValue* fields,
        int32_t protocol_version)
{
    if (__bolt_log_file==NULL) return;
    fprintf(__bolt_log_file, "bolt: %s[%" PRIu64 "]: ", peer, request_id);
    switch (protocol_version) {
    case 1:
    case 2: {
        const char* name = BoltProtocolV1_message_name(code);
        if (name==NULL) {
            fprintf(__bolt_log_file, "?");
        }
        else {
            fprintf(__bolt_log_file, "%s", name);
        }
        break;
    }
    default:
        fprintf(__bolt_log_file, "?");
    }
    for (int i = 0; i<fields->size; i++) {
        fprintf(__bolt_log_file, " ");
        BoltValue_write(BoltList_value(fields, i), __bolt_log_file, protocol_version);
    }
    fprintf(__bolt_log_file, "\n");
}
