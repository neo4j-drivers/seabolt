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

struct BoltAddress {
    /// Original host name or IP address string
    const char* host;
    /// Original service name or port number string
    const char* port;

    /// Number of resolved IP addresses
    int n_resolved_hosts;
    /// Resolved IP address data
    struct sockaddr_storage* resolved_hosts;
    /// Resolved port number
    uint16_t resolved_port;

    // Lock to protect DNS resolution process
    void* lock;
};

#ifdef __cplusplus
#define BoltAddress_of(host, port) { (const char *)host, (const char *)port, 0, nullptr, 0, nullptr }
#else
#define BoltAddress_of(host, port) (BoltAddress) { (const char *)host, (const char *)port, 0, NULL, 0, NULL }
#endif

BoltAddress* BoltAddress_create_from_string(const char* endpoint_str, uint64_t endpoint_len);

#endif //SEABOLT_ADDRESS_PRIVATE_H
