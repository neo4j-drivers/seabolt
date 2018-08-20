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

#ifndef SEABOLT_ALL_UTILS_H
#define SEABOLT_ALL_UTILS_H

#include "config.h"
#include <time.h>

struct StringBuilder {
    char* buffer;
    size_t buffer_size;
    size_t buffer_pos;
};

PUBLIC struct StringBuilder* StringBuilder_create();

PUBLIC void StringBuilder_destroy(struct StringBuilder* builder);

PUBLIC void StringBuilder_append(struct StringBuilder* builder, const char* string);

PUBLIC void StringBuilder_append_n(struct StringBuilder* builder, const char* string, const size_t len);

PUBLIC void StringBuilder_append_f(struct StringBuilder* builder, const char* format, ...);

PUBLIC char* StringBuilder_get_string(struct StringBuilder* builder);

PUBLIC size_t StringBuilder_get_length(struct StringBuilder* builder);

PUBLIC void BoltUtil_diff_time(struct timespec* t, struct timespec* t0, struct timespec* t1);

#endif //SEABOLT_ALL_UTILS_H
