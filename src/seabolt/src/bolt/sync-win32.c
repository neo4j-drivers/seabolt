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
#include "bolt-private.h"
#include "sync.h"
#include "mem.h"
#include "time.h"

void BoltSync_sleep(int milliseconds)
{
    SleepEx(milliseconds, TRUE);
}

int BoltSync_mutex_create(mutex_t* mutex)
{
    *mutex = BoltMem_allocate(sizeof(CRITICAL_SECTION));
    InitializeCriticalSectionAndSpinCount(*mutex, 0x400);
    return 0;
}

int BoltSync_mutex_destroy(mutex_t* mutex)
{
    DeleteCriticalSection(*mutex);
    BoltMem_deallocate(*mutex, sizeof(CRITICAL_SECTION));
    return 0;
}

int BoltSync_mutex_lock(mutex_t* mutex)
{
    EnterCriticalSection(*mutex);
    return 0;
}

int BoltSync_mutex_unlock(mutex_t* mutex)
{
    LeaveCriticalSection(*mutex);
    return 0;
}

int BoltSync_mutex_trylock(mutex_t* mutex)
{
    const BOOL result = TryEnterCriticalSection(*mutex);
    if (result) {
        return 1;
    }
    return 0;
}

int BoltSync_rwlock_create(rwlock_t* rwlock)
{
    InitializeSRWLock((PSRWLOCK) rwlock);
    return 1;
}

int BoltSync_rwlock_destroy(rwlock_t* rwlock)
{
    UNUSED(rwlock);
    return 1;
}

int BoltSync_rwlock_rdlock(rwlock_t* rwlock)
{
    AcquireSRWLockShared((PSRWLOCK) rwlock);
    return 1;
}

int BoltSync_rwlock_wrlock(rwlock_t* rwlock)
{
    AcquireSRWLockExclusive((PSRWLOCK) rwlock);
    return 1;
}

int BoltSync_rwlock_tryrdlock(rwlock_t* rwlock)
{
    return TryAcquireSRWLockShared((PSRWLOCK) rwlock)==TRUE;
}

int BoltSync_rwlock_trywrlock(rwlock_t* rwlock)
{
    return TryAcquireSRWLockExclusive((PSRWLOCK) rwlock)==TRUE;
}

int BoltSync_rwlock_timedrdlock(rwlock_t* rwlock, int timeout_ms)
{
    int64_t end_at = BoltTime_get_time_ms()+timeout_ms;

    while (1) {
        if (BoltSync_rwlock_tryrdlock(rwlock)) {
            return 1;
        }

        if (BoltTime_get_time_ms()>end_at) {
            return 0;
        }

        BoltSync_sleep(10);
    }
}

int BoltSync_rwlock_timedwrlock(rwlock_t* rwlock, int timeout_ms)
{
    int64_t end_at = BoltTime_get_time_ms()+timeout_ms;

    while (1) {
        if (BoltSync_rwlock_trywrlock(rwlock)) {
            return 1;
        }

        if (BoltTime_get_time_ms()>end_at) {
            return 0;
        }

        BoltSync_sleep(10);
    }
}

int BoltSync_rwlock_rdunlock(rwlock_t* rwlock)
{
    ReleaseSRWLockShared((PSRWLOCK) rwlock);
    return 1;
}

int BoltSync_rwlock_wrunlock(rwlock_t* rwlock)
{
    ReleaseSRWLockExclusive((PSRWLOCK) rwlock);
    return 1;
}

int BoltSync_cond_create(cond_t* cond)
{
    *cond = BoltMem_allocate(sizeof(CONDITION_VARIABLE));
    InitializeConditionVariable((PCONDITION_VARIABLE)*cond);
    return 1;
}

int BoltSync_cond_destroy(cond_t* cond)
{
    BoltMem_deallocate(*cond, sizeof(CONDITION_VARIABLE));
    return 1;
}

int BoltSync_cond_signal(cond_t* cond)
{
    WakeConditionVariable(*cond);
    return 1;
}

int BoltSync_cond_broadcast(cond_t* cond)
{
    WakeAllConditionVariable(*cond);
    return 1;
}

int BoltSync_cond_wait(cond_t* cond, mutex_t* mutex)
{
    return SleepConditionVariableCS(*cond, *mutex, INFINITE);
}

int BoltSync_cond_timedwait(cond_t* cond, mutex_t* mutex, int timeout_ms)
{
    return SleepConditionVariableCS(*cond, *mutex, timeout_ms);
}

unsigned long BoltThread_id()
{
    return (unsigned long) GetCurrentThreadId();
}