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
#include "platform.h"
#include "mem.h"

#ifdef __APPLE__
#include <mach/clock.h>
#include <mach/mach.h>
#include <libkern/OSAtomic.h>
#endif

#ifndef _WIN32

#include <pthread.h>
#include <assert.h>
#include <sys/time.h>

#endif

#define NANOSECONDS_PER_SEC 1000000000

int BoltUtil_get_time(struct timespec* tp)
{
#if defined(__APPLE__)
    clock_serv_t cclock;
    mach_timespec_t mts;
    host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &cclock);
    clock_get_time(cclock, &mts);
    mach_port_deallocate(mach_task_self(), cclock);
    tp->tv_sec = mts.tv_sec;
    tp->tv_nsec = mts.tv_nsec;
    return 0;
#elif defined(_WIN32)
    __int64 wintime;
    GetSystemTimePreciseAsFileTime((FILETIME*) &wintime);
    wintime -= 116444736000000000L;  //1jan1601 to 1jan1970
    tp->tv_sec = wintime/10000000L;           //seconds
    tp->tv_nsec = wintime%10000000L*100;      //nano-seconds
    return 0;
#else
    return timespec_get(tp, TIME_UTC);
#endif
}

int64_t BoltUtil_get_time_ms_from(struct timespec* tp)
{
    return (tp->tv_sec)*1000+(tp->tv_nsec)/1000000;
}

int64_t BoltUtil_get_time_ms()
{
    struct timespec now;
    BoltUtil_get_time(&now);
    return BoltUtil_get_time_ms_from(&now);
}

int64_t BoltUtil_increment(volatile int64_t* ref)
{
#if defined(__APPLE__)
    return OSAtomicIncrement64(ref);
#elif defined(_WIN32)
    return _InterlockedIncrement64(ref);
#else
    return __sync_add_and_fetch(ref, 1);
#endif
}

int64_t BoltUtil_decrement(volatile int64_t* ref)
{
#if defined(__APPLE__)
    return OSAtomicDecrement64(ref);
#elif defined(_WIN32)
    return _InterlockedDecrement64(ref);
#else
    return __sync_add_and_fetch(ref, -1);
#endif
}

int64_t BoltUtil_add(volatile int64_t* ref, int64_t by)
{
#if defined(__APPLE__)
    return OSAtomicAdd64(by, ref);
#elif defined(_WIN32)
    return _InterlockedExchangeAdd64(ref, by);
#else
    return __sync_add_and_fetch(ref, by);
#endif
}

void BoltUtil_sleep(int milliseconds)
{
#ifdef _WIN32
    SleepEx(milliseconds, TRUE);
#else
    usleep(milliseconds*1000);
#endif
}

int BoltUtil_mutex_create(mutex_t* mutex)
{
#ifdef _WIN32
    *mutex = BoltMem_allocate(sizeof(CRITICAL_SECTION));
    InitializeCriticalSectionAndSpinCount(*mutex, 0x400);
    return 0;
#else
    *mutex = BoltMem_allocate(sizeof(pthread_mutex_t));

    pthread_mutexattr_t mutex_attr;
    pthread_mutexattr_init(&mutex_attr);
    pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_RECURSIVE);

    return pthread_mutex_init(*mutex, &mutex_attr);
#endif
}

int BoltUtil_mutex_destroy(mutex_t* mutex)
{
#ifdef _WIN32
    DeleteCriticalSection(*mutex);
    BoltMem_deallocate(*mutex, sizeof(CRITICAL_SECTION));
    return 0;
#else
    int status = pthread_mutex_destroy(*mutex);
    BoltMem_deallocate(*mutex, sizeof(pthread_mutex_t));
    return status;
#endif
}

int BoltUtil_mutex_lock(mutex_t* mutex)
{
#ifdef _WIN32
    EnterCriticalSection(*mutex);
    return 0;
#else
    return pthread_mutex_lock(*mutex);
#endif
}

int BoltUtil_mutex_unlock(mutex_t* mutex)
{
#ifdef _WIN32
    LeaveCriticalSection(*mutex);
    return 0;
#else
    return pthread_mutex_unlock(*mutex);
#endif
}

int BoltUtil_mutex_trylock(mutex_t* mutex)
{
#ifdef _WIN32
    const BOOL result = TryEnterCriticalSection(*mutex);
    if (result) {
        return 1;
    }
    return 0;
#else
    return pthread_mutex_trylock(*mutex)==0;
#endif
}

int BoltUtil_rwlock_create(rwlock_t* rwlock)
{
#ifdef  _WIN32
    InitializeSRWLock((PSRWLOCK) rwlock);
    return 1;
#else
    *rwlock = BoltMem_allocate(sizeof(pthread_rwlock_t));

    pthread_rwlockattr_t rwlock_attr;
    pthread_rwlockattr_init(&rwlock_attr);

    return pthread_rwlock_init(*rwlock, &rwlock_attr)==0;
#endif
}

int BoltUtil_rwlock_destroy(rwlock_t* rwlock)
{
#ifdef  _WIN32
    UNUSED(rwlock);
    return 1;
#else
    int status = pthread_rwlock_destroy(*rwlock)==0;
    BoltMem_deallocate(*rwlock, sizeof(pthread_rwlock_t));
    return status;
#endif
}

int BoltUtil_rwlock_rdlock(rwlock_t* rwlock)
{
#ifdef  _WIN32
    AcquireSRWLockShared((PSRWLOCK) rwlock);
    return 1;
#else
    return pthread_rwlock_rdlock(*rwlock)==0;
#endif
}

int BoltUtil_rwlock_wrlock(rwlock_t* rwlock)
{
#ifdef  _WIN32
    AcquireSRWLockExclusive((PSRWLOCK) rwlock);
    return 1;
#else
    return pthread_rwlock_wrlock(*rwlock)==0;
#endif
}

int BoltUtil_rwlock_tryrdlock(rwlock_t* rwlock)
{
#ifdef  _WIN32
    return TryAcquireSRWLockShared((PSRWLOCK) rwlock)==TRUE;
#else
    return pthread_rwlock_tryrdlock(*rwlock)==0;
#endif
}

int BoltUtil_rwlock_trywrlock(rwlock_t* rwlock)
{
#ifdef  _WIN32
    return TryAcquireSRWLockExclusive((PSRWLOCK) rwlock)==TRUE;
#else
    return pthread_rwlock_trywrlock(*rwlock)==0;
#endif
}

int BoltUtil_rwlock_timedrdlock(rwlock_t* rwlock, int timeout_ms)
{
    int64_t end_at = BoltUtil_get_time_ms()+timeout_ms;

    while (1) {
        if (BoltUtil_rwlock_tryrdlock(rwlock)) {
            return 1;
        }

        if (BoltUtil_get_time_ms()>end_at) {
            return 0;
        }

        BoltUtil_sleep(10);
    }
}

int BoltUtil_rwlock_timedwrlock(rwlock_t* rwlock, int timeout_ms)
{
    int64_t end_at = BoltUtil_get_time_ms()+timeout_ms;

    while (1) {
        if (BoltUtil_rwlock_trywrlock(rwlock)) {
            return 1;
        }

        if (BoltUtil_get_time_ms()>end_at) {
            return 0;
        }

        BoltUtil_sleep(10);
    }
}

int BoltUtil_rwlock_rdunlock(rwlock_t* rwlock)
{
#ifdef  _WIN32
    ReleaseSRWLockShared((PSRWLOCK) rwlock);
    return 1;
#else
    return pthread_rwlock_unlock(*rwlock)==0;
#endif
}

int BoltUtil_rwlock_wrunlock(rwlock_t* rwlock)
{
#ifdef  _WIN32
    ReleaseSRWLockExclusive((PSRWLOCK) rwlock);
    return 1;
#else
    return pthread_rwlock_unlock(*rwlock)==0;
#endif
}

int BoltUtil_cond_create(cond_t* cond)
{
#ifdef _WIN32
    *cond = BoltMem_allocate(sizeof(CONDITION_VARIABLE));
    InitializeConditionVariable((PCONDITION_VARIABLE) *cond);
    return 1;
#else
    *cond = BoltMem_allocate(sizeof(pthread_cond_t));
    return pthread_cond_init(*cond, NULL)==0;
#endif
}

int BoltUtil_cond_destroy(cond_t* cond)
{
#ifdef _WIN32
    BoltMem_deallocate(*cond, sizeof(CONDITION_VARIABLE));
    return 1;
#else
    int status = pthread_cond_destroy(*cond)==0;
    BoltMem_deallocate(*cond, sizeof(pthread_cond_t));
    return status;
#endif
}

int BoltUtil_cond_signal(cond_t* cond)
{
#ifdef _WIN32
    WakeConditionVariable(*cond);
    return 1;
#else
    return pthread_cond_signal(*cond)==0;
#endif
}

int BoltUtil_cond_broadcast(cond_t* cond)
{
#ifdef _WIN32
    WakeAllConditionVariable(*cond);
    return 1;
#else
    return pthread_cond_broadcast(*cond)==0;
#endif
}

int BoltUtil_cond_wait(cond_t* cond, mutex_t* mutex)
{
#ifdef _WIN32
    return SleepConditionVariableCS(*cond, *mutex, INFINITE);
#else
    return pthread_cond_wait(*cond, *mutex);
#endif
}

int BoltUtil_cond_timedwait(cond_t* cond, mutex_t* mutex, int timeout_ms)
{
#ifdef _WIN32
    return SleepConditionVariableCS(*cond, *mutex, timeout_ms);
#else
    struct timeval now;
    struct timespec timeout;

    gettimeofday(&now, NULL);
    timeout.tv_sec = now.tv_sec+(timeout_ms/1000);
    timeout.tv_nsec = (now.tv_usec*1000)+((timeout_ms%1000)*1000000);
    if (timeout.tv_nsec>=NANOSECONDS_PER_SEC) {
        timeout.tv_sec++;
        timeout.tv_nsec -= NANOSECONDS_PER_SEC;
    }
    return pthread_cond_timedwait(*cond, *mutex, &timeout)==0;
#endif
}

