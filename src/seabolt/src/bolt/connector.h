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
#ifndef SEABOLT_ALL_CONNECTOR_H
#define SEABOLT_ALL_CONNECTOR_H

#include "bolt-public.h"
#include "log.h"
#include "connection.h"
#include "address-resolver.h"

/**
 * The intended access mode of the connection.
 *
 * Only applies to connectors that are configured to operate on a \ref BOLT_MODE_ROUTING mode.
 */
typedef int32_t BoltAccessMode;
/**
 * Use BOLT_ACCESS_MODE_WRITE to acquire a connection against a WRITE server.
 */
#define BOLT_ACCESS_MODE_WRITE  0
/**
 * Use BOLT_ACCESS_MODE_READ to acquire a connection against a READ server.
 */
#define BOLT_ACCESS_MODE_READ   1

/**
 * The type that holds a pool(s) of connections and handles connection acquisition and release operations based
 * on the configuration provided.
 *
 * An instance needs to be created with \ref BoltConnector_create and destroyed with \ref BoltConnector_destroy
 * when there is no more interaction is expected.
 */
typedef struct BoltConnector BoltConnector;

/**
 * Creates a new instance of \ref BoltConnector.
 *
 * @param address the \ref BoltAddress "address instance" that holds the target endpoint.
 * @param auth_token the authentication token to present to the server during connection initialisation.
 * @param config the \ref BoltConfig "configuration" that should apply to the created instance.
 * @return the pointer to the newly allocated \ref BoltConnector instance.
 */
SEABOLT_EXPORT BoltConnector*
BoltConnector_create(BoltAddress* address, BoltValue* auth_token, BoltConfig* config);

/**
 * Destroys the passed \ref BoltConnector instance and all related resources (connections, pools, etc.).
 *
 * @param connector the instance to be destroyed.
 */
SEABOLT_EXPORT void BoltConnector_destroy(BoltConnector* connector);

/**
 * Acquires a \ref BoltConnection "connection" from the underlying connection pool(s) based on the
 * provided \ref BoltAccessMode "access mode".
 *
 * It is the first call of this function that actual connection attempt is made towards the address provided.
 * If the connector is created with a \ref BOLT_MODE_ROUTING mode, this will cause an initial discovery to be
 * carried out against the cluster member pointed by the address.
 * In case a custom name resolver is specified through \ref BoltConfig_set_address_resolver, the initial and fallback
 * discoveries trigger this name resolver and the returned addresses will be used for routing table updates.
 *
 * @param connector the instance to acquire connection from.
 * @param mode the intended \ref BoltAccessMode "access mode" for the acquired connection.
 * @param status the status about the acquisition, which is mostly useful if something went wrong and no connection
 * could be acquired
 * @returns the acquired connection or NULL if something went wrong. Inspect status for more detailed information about
 * the underlying failure.
 */
SEABOLT_EXPORT BoltConnection* BoltConnector_acquire(BoltConnector* connector, BoltAccessMode mode, BoltStatus* status);

/**
 * Releases the provided \ref BoltConnection "connection" to the internal connection pool to be used by further
 * \ref BoltConnector_acquire calls.
 *
 * @param connector the instance to release the connection to.
 * @param connection the actual \ref BoltConnection "connection" to be released.
 */
SEABOLT_EXPORT void BoltConnector_release(BoltConnector* connector, BoltConnection* connection);

#endif //SEABOLT_ALL_CONNECTOR_H
