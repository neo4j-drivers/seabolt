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
 * Destroys the passed \ref BoltAddress instance.
 *
 * @param address the instance to be destroyed.
 */
SEABOLT_EXPORT void BoltAddress_destroy(BoltAddress* address);

#endif // SEABOLT_ADDRESSING_H
