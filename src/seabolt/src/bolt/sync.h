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
#ifndef SEABOLT_SYNC_H
#define SEABOLT_SYNC_H

#include "bolt-public.h"

typedef void* mutex_t;

typedef void* rwlock_t;

typedef void* cond_t;

int BoltSync_mutex_create(mutex_t* mutex);

int BoltSync_mutex_destroy(mutex_t* mutex);

int BoltSync_mutex_lock(mutex_t* mutex);

int BoltSync_mutex_unlock(mutex_t* mutex);

int BoltSync_mutex_trylock(mutex_t* mutex);

int BoltSync_rwlock_create(rwlock_t* rwlock);

int BoltSync_rwlock_destroy(rwlock_t* rwlock);

int BoltSync_rwlock_rdlock(rwlock_t* rwlock);

int BoltSync_rwlock_wrlock(rwlock_t* rwlock);

int BoltSync_rwlock_tryrdlock(rwlock_t* rwlock);

int BoltSync_rwlock_trywrlock(rwlock_t* rwlock);

int BoltSync_rwlock_timedrdlock(rwlock_t* rwlock, int timeout_ms);

int BoltSync_rwlock_timedwrlock(rwlock_t* rwlock, int timeout_ms);

int BoltSync_rwlock_rdunlock(rwlock_t* rwlock);

int BoltSync_rwlock_wrunlock(rwlock_t* rwlock);

int BoltSync_cond_create(cond_t* cond);

int BoltSync_cond_destroy(cond_t* cond);

int BoltSync_cond_signal(cond_t* cond);

int BoltSync_cond_broadcast(cond_t* cond);

int BoltSync_cond_wait(cond_t* cond, mutex_t* mutex);

int BoltSync_cond_timedwait(cond_t* cond, mutex_t* mutex, int timeout_ms);

unsigned long BoltThread_id();

#endif //SEABOLT_SYNC_H
