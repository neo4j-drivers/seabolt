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


#ifndef SEABOLT_LOG
#define SEABOLT_LOG

#include "bolt-public.h"
#include "connection.h"
#include "values.h"

/**
 * The logging function signature that implementors should provide for a working logging.
 *
 * @param state the state object as provided to \ref BoltLog_create
 * @param message the actual log message to be processed by the implementor (i.e. print to console or file, etc.)
 */
typedef void (* log_func)(void* state, const char* message);

/**
 * The type that defines a logger to be used across the connector.
 *
 * An instance needs to be created with \ref BoltLog_create and destroyed with \ref BoltLog_create
 * either after \ref BoltConfig_set_log is called if using \ref BoltConnector, or after all
 * \ref BoltConnection instances explicitly opened with \ref BoltConnection_open are closed.
 */
typedef struct BoltLog BoltLog;

/**
 * Creates a new instance of \ref BoltLog.
 *
 * @param state an optional object that will be passed to provided logging functions
 * @returns the pointer to the newly allocated \ref BoltLog instance.
 */
SEABOLT_EXPORT BoltLog* BoltLog_create(void* state);

/**
 * Destroys the passed \ref BoltLog instance.
 *
 * @param log the instance to be destroyed.
 */
SEABOLT_EXPORT void BoltLog_destroy(BoltLog* log);

/**
 * Sets the logging function to be used for logging ERROR level messages.
 *
 * @param log the instance to be modified.
 * @param func the log function to be set.
 */
SEABOLT_EXPORT void BoltLog_set_error_func(BoltLog* log, log_func func);

/**
 * Sets the logging function to be used for logging WARNING level messages.
 *
 * @param log the instance to be modified.
 * @param func the log function to be set.
 */
SEABOLT_EXPORT void BoltLog_set_warning_func(BoltLog* log, log_func func);

/**
 * Sets the logging function to be used for logging INFO level messages.
 *
 * @param log the instance to be modified.
 * @param func the log function to be set.
 */
SEABOLT_EXPORT void BoltLog_set_info_func(BoltLog* log, log_func func);

/**
 * Sets the logging function to be used for logging DEBUG level messages.
 *
 * @param log the instance to be modified.
 * @param func the log function to be set.
 */
SEABOLT_EXPORT void BoltLog_set_debug_func(BoltLog* log, log_func func);

#endif // SEABOLT_LOG
