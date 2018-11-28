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

#ifndef SEABOLT_ADDRESSING_H
#define SEABOLT_ADDRESSING_H

#include "bolt-public.h"

/**
 * The type that represents a network endpoint as a host name and a port number.
 *
 * This can carry both the original host name and port details, as supplied by the application,
 * as well as one or more resolved IP addresses and port number.
 */
typedef struct BoltAddress BoltAddress;

/**
 * Creates a new instance of \ref BoltAddress for a given host and port.
 *
 * No name resolution is carried out on creation so this simply initialises
 * the original host and port details and zeroes out the remainder
 * of the structure.
 *
 * @param host host name, e.g. "www.example.com"
 * @param port port name or numeric string, e.g. "7687" or "http"
 * @returns the pointer to the newly allocated \ref BoltAddress instance.
 */
SEABOLT_EXPORT BoltAddress* BoltAddress_create(const char* host, const char* port);

/**
 * Returns the host name.
 *
 * @param address the instance to be queried.
 * @returns the host name as specified with \ref BoltAddress_create.
 */
SEABOLT_EXPORT const char* BoltAddress_host(BoltAddress* address);

/**
 * Returns the port.
 *
 * @param address the instance to be queried.
 * @returns the port as specified with \ref BoltAddress_create.
 */
SEABOLT_EXPORT const char* BoltAddress_port(BoltAddress* address);

/**
 * Resolves the original host and port into one or more IP addresses and
 * a port number.
 *
 * This can be carried out more than once on the same
 * address. Any newly-resolved addresses will replace any previously stored.
 *
 * The name resolution is a synchronized operation, i.e. concurrent resolution requests on the same
 * instance are protected by a mutex.
 *
 * @param address the instance to be resolved.
 * @param n_resolved number of resolved addresses that will be set upon successful resolution.
 * @param log an optional \ref BoltLog instance to be used for logging purposes.
 * @returns 0 for success, and non-zero error codes returned from getaddrinfo call on error.
 */
SEABOLT_EXPORT int32_t BoltAddress_resolve(BoltAddress* address, int32_t* n_resolved, BoltLog* log);

/**
 * Copies the textual representation of the resolved IP address at the specified index into an already
 * allocated buffer.
 *
 * If successful, AF_INET or AF_INET6 is returned depending on the address family. If unsuccessful, -1 is returned.
 * Failure may be a result of a system problem or because the supplied buffer is too small for the address.
 *
 * @param address the instance to be queried.
 * @param index index of the resolved IP address
 * @param buffer destination buffer to write the IP address's string representation
 * @param buffer_size size of the buffer
 * @return address family (AF_INET or AF_INET6) or -1 on error
 */
SEABOLT_EXPORT int32_t
BoltAddress_copy_resolved_host(BoltAddress* address, int32_t index, char* buffer, uint64_t buffer_size);

/**
 * Destroys the passed \ref BoltAddress instance.
 *
 * @param address the instance to be destroyed.
 */
SEABOLT_EXPORT void BoltAddress_destroy(BoltAddress* address);

#endif // SEABOLT_ADDRESSING_H
