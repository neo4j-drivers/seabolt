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
#ifndef SEABOLT_STATUS_H
#define SEABOLT_STATUS_H

#include "bolt-public.h"

/**
 * The type that identifies the state of the connection
 */
typedef int32_t BoltConnectionState;
/**
 * Not connected
 */
#define BOLT_CONNECTION_STATE_DISCONNECTED    0
/**
 * Connected but not authenticated
 */
#define BOLT_CONNECTION_STATE_CONNECTED       1
/**
 * Connected and authenticated
 */
#define BOLT_CONNECTION_STATE_READY           2
/**
 * Recoverable failure
 */
#define BOLT_CONNECTION_STATE_FAILED          3
/**
 * Unrecoverable failure
 */
#define BOLT_CONNECTION_STATE_DEFUNCT         4

/**
 * The type that holds status information about connection, including details about errors.
 */
typedef struct BoltStatus BoltStatus;

/**
 * Creates a new instance of \ref BoltStatus.
 *
 * @return the pointer to the newly allocated \ref BoltStatus instance.
 */
SEABOLT_EXPORT BoltStatus* BoltStatus_create();

/**
 * Destroys the passed \ref BoltStatus instance.
 *
 * @param status the instance to be destroyed.
 */
SEABOLT_EXPORT void BoltStatus_destroy(BoltStatus* status);

/**
 * Returns the current \ref BoltConnectionState "state".
 *
 * @param status the instance to be destroyed.
 * @returns the current \ref BoltConnectionState "state".
 */
SEABOLT_EXPORT BoltConnectionState BoltStatus_get_state(BoltStatus* status);

/**
 * Returns the current error code.
 *
 * A string representation of the returned error code can be retrieved by calling
 * \ref BoltError_get_string.
 *
 * @param status the instance to be destroyed.
 * @returns the current error code.
 */
SEABOLT_EXPORT int32_t BoltStatus_get_error(BoltStatus* status);

/**
 * Returns more information (if set by the internal code) about the error stored, like
 * which internal call generated the error and the location of it.
 *
 * @param status the instance to be destroyed.
 * @returns the current error context (may be NULL or empty string).
 */
SEABOLT_EXPORT const char* BoltStatus_get_error_context(BoltStatus* status);

#endif //SEABOLT_STATUS_H
