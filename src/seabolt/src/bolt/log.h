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


#ifndef SEABOLT_LOG
#define SEABOLT_LOG

#include "bolt-public.h"
#include "connection.h"
#include "values.h"

typedef void (* log_func)(int state, const char* message);

typedef struct BoltLog BoltLog;

SEABOLT_EXPORT struct BoltLog* BoltLog_create(int state);

SEABOLT_EXPORT void BoltLog_destroy(struct BoltLog* log);

SEABOLT_EXPORT void BoltLog_set_error_func(BoltLog* log, log_func func);

SEABOLT_EXPORT void BoltLog_set_warning_func(BoltLog* log, log_func func);

SEABOLT_EXPORT void BoltLog_set_info_func(BoltLog* log, log_func func);

SEABOLT_EXPORT void BoltLog_set_debug_func(BoltLog* log, log_func func);

#endif // SEABOLT_LOG
