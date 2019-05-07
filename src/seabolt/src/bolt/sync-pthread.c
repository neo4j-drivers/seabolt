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

#include <pthread.h>
#include <sys/time.h>

void BoltSync_sleep(int milliseconds)
{
    usleep(milliseconds*1000);
}

int BoltSync_mutex_create(mutex_t* mutex)
{
    *mutex = BoltMem_allocate(sizeof(pthread_mutex_t));

    pthread_mutexattr_t mutex_attr;
    pthread_mutexattr_init(&mutex_attr);
    pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_RECURSIVE);

    return pthread_mutex_init(*mutex, &mutex_attr);
}

int BoltSync_mutex_destroy(mutex_t* mutex)
{
    int status = pthread_mutex_destroy(*mutex);
    BoltMem_deallocate(*mutex, sizeof(pthread_mutex_t));
    return status;
}

int BoltSync_mutex_lock(mutex_t* mutex)
{
    return pthread_mutex_lock(*mutex);
}

int BoltSync_mutex_unlock(mutex_t* mutex)
{
    return pthread_mutex_unlock(*mutex);
}

int BoltSync_mutex_trylock(mutex_t* mutex)
{
    return pthread_mutex_trylock(*mutex)==0;
}

int BoltSync_rwlock_create(rwlock_t* rwlock)
{
    *rwlock = BoltMem_allocate(sizeof(pthread_rwlock_t));
    return pthread_rwlock_init(*rwlock, NULL)==0;
}

int BoltSync_rwlock_destroy(rwlock_t* rwlock)
{
    int status = pthread_rwlock_destroy(*rwlock)==0;
    BoltMem_deallocate(*rwlock, sizeof(pthread_rwlock_t));
    *rwlock = NULL;
    return status;
}

int BoltSync_rwlock_rdlock(rwlock_t* rwlock)
{
    return pthread_rwlock_rdlock(*rwlock)==0;
}

int BoltSync_rwlock_wrlock(rwlock_t* rwlock)
{
    return pthread_rwlock_wrlock(*rwlock)==0;
}

int BoltSync_rwlock_tryrdlock(rwlock_t* rwlock)
{
    return pthread_rwlock_tryrdlock(*rwlock)==0;
}

int BoltSync_rwlock_trywrlock(rwlock_t* rwlock)
{
    return pthread_rwlock_trywrlock(*rwlock)==0;
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
    return pthread_rwlock_unlock(*rwlock)==0;
}

int BoltSync_rwlock_wrunlock(rwlock_t* rwlock)
{
    return pthread_rwlock_unlock(*rwlock)==0;
}

int BoltSync_cond_create(cond_t* cond)
{
    *cond = BoltMem_allocate(sizeof(pthread_cond_t));
    return pthread_cond_init(*cond, NULL)==0;
}

int BoltSync_cond_destroy(cond_t* cond)
{
    int status = pthread_cond_destroy(*cond)==0;
    BoltMem_deallocate(*cond, sizeof(pthread_cond_t));
    return status;
}

int BoltSync_cond_signal(cond_t* cond)
{
    return pthread_cond_signal(*cond)==0;
}

int BoltSync_cond_broadcast(cond_t* cond)
{
    return pthread_cond_broadcast(*cond)==0;
}

int BoltSync_cond_wait(cond_t* cond, mutex_t* mutex)
{
    return pthread_cond_wait(*cond, *mutex);
}

int BoltSync_cond_timedwait(cond_t* cond, mutex_t* mutex, int timeout_ms)
{
    struct timeval now;
    struct timespec timeout;

    gettimeofday(&now, NULL);
    timeout.tv_sec = now.tv_sec+(timeout_ms/1000);
    timeout.tv_nsec = (now.tv_usec*1000)+((timeout_ms%1000)*1000000);
    if (timeout.tv_nsec>=NANOS_PER_SEC) {
        timeout.tv_sec++;
        timeout.tv_nsec -= NANOS_PER_SEC;
    }
    return pthread_cond_timedwait(*cond, *mutex, &timeout)==0;
}

unsigned long BoltThread_id()
{
    return (unsigned long) pthread_self();
}
