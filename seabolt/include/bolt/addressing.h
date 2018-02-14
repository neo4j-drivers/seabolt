/*
 * Copyright (c) 2002-2017 "Neo Technology,"
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

#ifndef SEABOLT_ADDRESSING_H
#define SEABOLT_ADDRESSING_H


#include "config.h"


/**
 * The address of a Bolt server. This can carry both the original host
 * and port details, as supplied by the application, as well as one or
 * more resolved IP addresses and port number.
 */
struct BoltAddress
{
    /// Original host name or IP address string
    const char * host;
    /// Original service name or port number string
    const char * port;

    /// Number of resolved IP addresses
    int n_resolved_hosts;
    /// Resolved IP address data
    struct sockaddr_storage * resolved_hosts;
    /// Resolved port number
    in_port_t resolved_port;
};

/**
 * Create a new address structure for a given host and port. No name
 * resolution is carried out on creation so this simply initialises
 * the original host and port details and zeroes out the remainder
 * of the structure.
 *
 * @param host host name for this address, e.g. "www.example.com"
 * @param port port name or numeric string, e.g. "7687" or "http"
 * @return pointer to a new BoltAddress structure
 */
PUBLIC struct BoltAddress * BoltAddress_create(const char * host, const char * port);

/**
 * Resolve the original host and port into one or more IP addresses and
 * a port number. This can be carried out more than once on the same
 * address any newly-resolved addresses will replace any previously stored.
 *
 * @param address pointer to a BoltAddress structure
 * @return status of the internal getaddrinfo call
 */
PUBLIC int BoltAddress_resolve_b(struct BoltAddress * address);

/**
 * Copy the textual representation of a resolved host IP address into a buffer.
 *
 * If successful, AF_INET or AF_INET6 is returned depending on the address
 * family. If unsuccessful, -1 is returned. Failure may be a result of a
 * system problem or because the supplied buffer is too small for the address.
 *
 * @param address pointer to a resolved BoltAddress structure
 * @param index index of the resolved IP address
 * @param buffer buffer in which to write the address representation
 * @param buffer_size size of the buffer
 * @return address family (AF_INET or AF_INET6) or -1 on error
 */
PUBLIC int BoltAddress_copy_resolved_host(struct BoltAddress * address, size_t index, char * buffer, socklen_t buffer_size);

/**
 * Destroy an address structure and deallocate any associated memory.
 *
 * @param address pointer to a BoltAddress structure
 */
PUBLIC void BoltAddress_destroy(struct BoltAddress * address);


#endif // SEABOLT_ADDRESSING_H
