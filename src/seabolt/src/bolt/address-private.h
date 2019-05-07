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
#ifndef SEABOLT_ADDRESS_PRIVATE_H
#define SEABOLT_ADDRESS_PRIVATE_H

#include "address.h"
#include "sync.h"

struct BoltAddress {
    /// Original host name or IP address string
    const char* host;
    /// Original service name or port number string
    const char* port;

    /// Number of resolved IP addresses
    volatile int n_resolved_hosts;
    /// Resolved IP address data
    volatile struct sockaddr_storage* resolved_hosts;
    /// Resolved port number
    uint16_t resolved_port;

    // Lock to protect DNS resolution process
    rwlock_t lock;
};

#ifdef __cplusplus
#define BoltAddress_of(host, port) { (const char *)host, (const char *)port, 0, nullptr, 0, nullptr }
#else
#define BoltAddress_of(host, port) (BoltAddress) { (const char *)host, (const char *)port, 0, NULL, 0, NULL }
#endif

BoltAddress* BoltAddress_create_with_lock(const char* host, const char* port);

BoltAddress* BoltAddress_create_from_string(const char* endpoint_str, uint64_t endpoint_len);

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
int32_t BoltAddress_resolve(BoltAddress* address, int32_t* n_resolved, BoltLog* log);

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
int32_t
BoltAddress_copy_resolved_host(BoltAddress* address, int32_t index, char* buffer, int32_t buffer_size);

/**
 * Returns the number of resolved addresses after call to BoltAddress_resolve.
 *
 * @param address the instance to be queried.
 * @return number of resolved entities.
 */
int32_t BoltAddress_resolved_count(BoltAddress* address);

/**
 * Copies the resolved address entity at index to the passed in target.
 *
 * @param address the instance to be queried.
 * @param index the index of the component to get.
 * @param target the target memory where the entity will be copied.
 * @return 1 on success, 0 otherwise.
 */
int BoltAddress_resolved_addr(BoltAddress* address, int32_t index, struct sockaddr_storage* target);

#endif //SEABOLT_ADDRESS_PRIVATE_H
