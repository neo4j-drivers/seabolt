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
#ifndef SEABOLT_ALL_ERROR_H
#define SEABOLT_ALL_ERROR_H

#include "bolt-public.h"

/// Identifies a successful operation which is defined as 0
#define BOLT_SUCCESS 0
/// Unknown error
#define BOLT_UNKNOWN_ERROR   1
/// Unsupported protocol or address family
#define BOLT_UNSUPPORTED   2
/// Operation interrupted
#define BOLT_INTERRUPTED   3
/// Connection reset by peer
#define BOLT_CONNECTION_RESET   4
/// No valid resolved addresses found to connect
#define BOLT_NO_VALID_ADDRESS   5
/// Operation timed out
#define BOLT_TIMED_OUT   6
/// Permission denied
#define BOLT_PERMISSION_DENIED   7
/// Too may open files
#define BOLT_OUT_OF_FILES   8
/// Out of memory
#define BOLT_OUT_OF_MEMORY   9
/// Too many open ports
#define BOLT_OUT_OF_PORTS   10
/// Connection refused
#define BOLT_CONNECTION_REFUSED   11
/// Network unreachable
#define BOLT_NETWORK_UNREACHABLE   12
/// An unknown TLS error
#define BOLT_TLS_ERROR   13
/// Connection closed by remote peer
#define BOLT_END_OF_TRANSMISSION   15
/// Server returned a FAILURE message, more info available through \ref BoltConnection_failure
#define BOLT_SERVER_FAILURE   16
/// Unsupported bolt transport
#define BOLT_TRANSPORT_UNSUPPORTED   0x400
/// Unsupported protocol usage
#define BOLT_PROTOCOL_VIOLATION   0x500
/// Unsupported bolt type
#define BOLT_PROTOCOL_UNSUPPORTED_TYPE   0x501
/// Unknown pack stream type
#define BOLT_PROTOCOL_NOT_IMPLEMENTED_TYPE   0x502
/// Unexpected marker
#define BOLT_PROTOCOL_UNEXPECTED_MARKER   0x503
/// Unsupported bolt protocol version
#define BOLT_PROTOCOL_UNSUPPORTED   0x504
/// Connection pool is full
#define BOLT_POOL_FULL   0x600
/// Connection acquisition from the connection pool timed out
#define BOLT_POOL_ACQUISITION_TIMED_OUT   0x601
/// Address resolution failed
#define BOLT_ADDRESS_NOT_RESOLVED   0x700
/// Address name info couldn't be looked up
#define BOLT_ADDRESS_NAME_INFO_FAILED   0x701
/// Routing table retrieval failed
#define BOLT_ROUTING_UNABLE_TO_RETRIEVE_ROUTING_TABLE   0x800
/// No servers to select for the requested operation
#define BOLT_ROUTING_NO_SERVERS_TO_SELECT   0x801
/// Connection pool construction for server failed
#define BOLT_ROUTING_UNABLE_TO_CONSTRUCT_POOL_FOR_SERVER   0x802
/// Routing table refresh failed
#define BOLT_ROUTING_UNABLE_TO_REFRESH_ROUTING_TABLE   0x803
/// Invalid discovery response
#define BOLT_ROUTING_UNEXPECTED_DISCOVERY_RESPONSE   0x804
/// Error set in connection
#define BOLT_CONNECTION_HAS_MORE_INFO   0xFFE
/// Error set in connection
#define BOLT_STATUS_SET   0xFFF

/**
 * Returns a human readable description of the passed error _code_.
 *
 * @param code the error code
 * @returns the null terminated string buffer.
 */
SEABOLT_EXPORT const char* BoltError_get_string(int32_t code);

#endif //SEABOLT_ALL_ERROR_H
