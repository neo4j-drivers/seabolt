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

#include "logging.h"
#include "mem.h"
#include "v1.h"
#include "string-builder.h"

struct BoltLog* BoltLog_create()
{
    struct BoltLog* log = (struct BoltLog*) BoltMem_allocate(sizeof(struct BoltLog));
    log->state = 0;
    log->debug_logger = NULL;
    log->info_logger = NULL;
    log->warning_logger = NULL;
    log->error_logger = NULL;
    log->debug_enabled = 0;
    log->info_enabled = 0;
    log->warning_enabled = 0;
    log->error_enabled = 0;
    return log;
}

void BoltLog_destroy(struct BoltLog* log)
{
    BoltMem_deallocate(log, sizeof(struct BoltLog));
}

void _perform_log_call(log_func func, int state, const char* format, va_list args)
{
    size_t
            size = 512*sizeof(char);
    char* message_fmt = (char*) BoltMem_allocate(size);
    while (1) {
        va_list args_copy;
        va_copy(args_copy, args);
        size_t
                written = vsnprintf(message_fmt, size, format, args_copy);
        va_end(args_copy);
        if (written<size) {
            break;
        }

        message_fmt = (char*) BoltMem_reallocate(message_fmt, size, written+1);
        size = written+1;
    }

    func(state, message_fmt);

    BoltMem_deallocate(message_fmt, size);
}

void BoltLog_error(const struct BoltLog* log, const char* format, ...)
{
    if (log!=NULL && log->error_enabled) {
        va_list args;
        va_start(args, format);
        _perform_log_call(log->error_logger, log->state, format, args);
        va_end(args);
    }
}

void BoltLog_warning(const struct BoltLog* log, const char* format, ...)
{
    if (log!=NULL && log->warning_enabled) {
        va_list args;
        va_start(args, format);
        _perform_log_call(log->warning_logger, log->state, format, args);
        va_end(args);
    }
}

void BoltLog_info(const struct BoltLog* log, const char* format, ...)
{
    if (log!=NULL && log->info_enabled) {
        va_list args;
        va_start(args, format);
        _perform_log_call(log->info_logger, log->state, format, args);
        va_end(args);

    }
}

void BoltLog_debug(const struct BoltLog* log, const char* format, ...)
{
    if (log!=NULL && log->debug_enabled) {
        va_list args;
        va_start(args, format);
        _perform_log_call(log->debug_logger, log->state, format, args);
        va_end(args);
    }
}

void
BoltLog_value(const struct BoltLog* log, const char* format, const char* id, struct BoltValue* value,
        name_resolver_func struct_name_resolver)
{
    if (log!=NULL && log->debug_enabled) {
        struct StringBuilder* builder = StringBuilder_create();
        BoltValue_write(builder, value, struct_name_resolver);
        BoltLog_debug(log, format, id, StringBuilder_get_string(builder));
        StringBuilder_destroy(builder);
    }
}

void BoltLog_message(const struct BoltLog* log, const char* id, const char* peer, bolt_request request_id, int16_t code,
        struct BoltValue* fields, name_resolver_func struct_name_resolver, name_resolver_func message_name_resolver)
{
    if (log!=NULL && log->debug_enabled) {
        const char* message_name = "?";
        if (message_name_resolver!=NULL) {
            message_name = message_name_resolver(code);
        }

        struct StringBuilder* builder = StringBuilder_create();
        BoltValue_write(builder, fields, struct_name_resolver);
        BoltLog_debug(log, "[%s]: %s[%" PRIu64 "] %s %s", id, peer, request_id, message_name,
                StringBuilder_get_string(builder));
        StringBuilder_destroy(builder);
    }
}
