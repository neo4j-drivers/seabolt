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
#ifndef SEABOLT_ALL_TLS_H
#define SEABOLT_ALL_TLS_H

#include "config-impl.h"
#include "logging.h"

PUBLIC struct ssl_ctx_st* create_ssl_ctx(struct BoltTrust* trust, const char* hostname, const struct BoltLog* log);

PUBLIC void free_ssl_context(struct ssl_ctx_st* ctx);

#endif //SEABOLT_ALL_TLS_H
