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
#ifndef SEABOLT_TIME_H
#define SEABOLT_TIME_H

#include "bolt-public.h"
#include <time.h>

#ifndef NANOS_PER_SEC
#define NANOS_PER_SEC 1000000000
#endif

SEABOLT_EXPORT int BoltTime_get_time(struct timespec* tp);

int64_t BoltTime_get_time_ms();

int64_t BoltTime_get_time_ms_from(struct timespec* tp);

void BoltTime_diff_time(struct timespec* t, struct timespec* t0, struct timespec* t1);

#endif //SEABOLT_TIME_H
