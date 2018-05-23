/*
* Copyright (c) 2002-2018 "Neo Technology,"
* Network Engine for Objects in Lund AB [http://neotechnology.com]
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

#include "bolt/config-impl.h"
#include "bolt/platform.h"
#include <time.h>

#ifdef __APPLE__
#include <mach/clock.h>
#include <mach/mach.h>
#endif

#ifndef _WIN32
#include <pthread.h>
#endif

int BoltUtil_get_time(struct timespec *tp)
{
#ifdef __APPLE__
	clock_serv_t cclock;
	mach_timespec_t mts;
	host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &cclock);
	clock_get_time(cclock, &mts);
	mach_port_deallocate(mach_task_self(), cclock);
	ts->tv_sec = mts.tv_sec;
	ts->tv_nsec = mts.tv_nsec;
	return 0;
#else
	return timespec_get(tp, TIME_UTC);
#endif
}

int BoltUtil_mutex_create(mutex_t *mutex)
{
#ifdef _WIN32
	*mutex = CreateMutex(NULL, FALSE, NULL);
	if (*mutex == NULL)
	{
		return GetLastError();
	}
	return 0;
#else
	return pthread_mutex_init(mutex, NULL);
#endif
}

int BoltUtil_mutex_destroy(mutex_t *mutex)
{
#ifdef _WIN32
	if (CloseHandle(*mutex))
	{
		return GetLastError();
	}
	return 0;
#else
	return pthread_mutex_destroy(mutex);
#endif
}

int BoltUtil_mutex_lock(mutex_t *mutex)
{
#ifdef _WIN32
	const DWORD result = WaitForSingleObject(*mutex, INFINITE);
	if (result)
	{
		return result;
	}
	return 0;
#else
	return pthread_mutex_lock(mutex);
#endif
}

int BoltUtil_mutex_unlock(mutex_t *mutex)
{
#ifdef _WIN32
	if (ReleaseMutex(*mutex))
	{
		return GetLastError();
	}

	return 0;
#else
	return pthread_mutex_unlock(mutex);
#endif
}

int BoltUtil_mutex_trylock(mutex_t *mutex)
{
#ifdef _WIN32
	const DWORD result = WaitForSingleObject(*mutex, 0);
	if (result)
	{
		return result;
	}
	return 0;
#else
	return pthread_mutex_trylock(mutex);
#endif
}