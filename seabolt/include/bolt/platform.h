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

#ifndef SEABOLT_UTILS_H
#define SEABOLT_UTILS_H

#include "config.h"
#include <time.h>

#if  _WIN32
typedef void * mutex_t;
#else
typedef pthread_mutex_t mutex_t;
#endif

#if _WIN32

#else
typedef pthread_rwlock_t rwlock_t;
#endif

PUBLIC int BoltUtil_get_time(struct timespec* tp);

int64_t BoltUtil_get_time_ms();

int BoltUtil_increment(volatile int* ref);

int BoltUtil_decrement(volatile int* ref);

int BoltUtil_mutex_create(mutex_t* mutex);

int BoltUtil_mutex_destroy(mutex_t* mutex);

int BoltUtil_mutex_lock(mutex_t* mutex);

int BoltUtil_mutex_unlock(mutex_t* mutex);

int BoltUtil_mutex_trylock(mutex_t* mutex);

int BoltUtil_rwlock_create(rwlock_t* rwlock);

int BoltUtil_rwlock_destroy(rwlock_t* rwlock);

int BoltUtil_rwlock_rdlock(rwlock_t* rwlock);

int BoltUtil_rwlock_wrlock(rwlock_t* rwlock);

int BoltUtil_rwlock_tryrdlock(rwlock_t* rwlock);

int BoltUtil_rwlock_trywrlock(rwlock_t* rwlock);

int BoltUtil_rwlock_timedrdlock(rwlock_t* rwlock, int timeout_ms);

int BoltUtil_rwlock_timedwrlock(rwlock_t* rwlock, int timeout_ms);

int BoltUtil_rwlock_unlock(rwlock_t* rwlock);

#endif //SEABOLT_UTILS_H

