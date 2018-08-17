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


#ifndef SEABOLT_TEST
#define SEABOLT_TEST

#include <cstring>
#include <cstdlib>

// This is to get PRIi64 type printf format specifiers get to work in C++
#define __STDC_FORMAT_MACROS

extern "C" {
#include "bolt/config-options.h"

// Override OPENSSL usage so that the related headers are not included
#ifdef USE_OPENSSL
#undef USE_OPENSSL
#define USE_OPENSSL 0
#endif

#include "bolt/logging.h"
#include "bolt/platform.h"
#include "bolt/connections.h"

#include "src/bolt/protocol/v1.h"

#include "src/bolt/addressing.c"
#include "src/bolt/values.c"
#include "src/bolt/memory.c"
#include "src/bolt/utils/address-set.c"
#include "src/bolt/pool/routing-table.c"
#include "src/bolt/pool/direct-pool.c"
#include "src/bolt/pool/routing-pool.c"
}

#endif // SEABOLT_TEST

