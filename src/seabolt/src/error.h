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
#ifndef SEABOLT_ALL_ERROR_H
#define SEABOLT_ALL_ERROR_H

#include "config.h"

#define BOLT_SUCCESS 0
#define BOLT_UNKNOWN_ERROR   1
#define BOLT_UNSUPPORTED   2
#define BOLT_INTERRUPTED   3
#define BOLT_CONNECTION_RESET   4
#define BOLT_NO_VALID_ADDRESS   5
#define BOLT_TIMED_OUT   6
#define BOLT_PERMISSION_DENIED   7
#define BOLT_OUT_OF_FILES   8
#define BOLT_OUT_OF_MEMORY   9
#define BOLT_OUT_OF_PORTS   10
#define BOLT_CONNECTION_REFUSED   11
#define BOLT_NETWORK_UNREACHABLE   12
#define BOLT_TLS_ERROR   13
#define BOLT_END_OF_TRANSMISSION   15
#define BOLT_SERVER_FAILURE   16
#define BOLT_TRANSPORT_UNSUPPORTED   0x400
#define BOLT_PROTOCOL_VIOLATION   0x500
#define BOLT_PROTOCOL_UNSUPPORTED_TYPE   0x501
#define BOLT_PROTOCOL_NOT_IMPLEMENTED_TYPE   0x502
#define BOLT_PROTOCOL_UNEXPECTED_MARKER   0x503
#define BOLT_PROTOCOL_UNSUPPORTED   0x504
#define BOLT_POOL_FULL   0x600
#define BOLT_POOL_ACQUISITION_TIMED_OUT   0x601
#define BOLT_ADDRESS_NOT_RESOLVED   0x700
#define BOLT_ROUTING_UNABLE_TO_RETRIEVE_ROUTING_TABLE   0x800
#define BOLT_ROUTING_NO_SERVERS_TO_SELECT   0x801
#define BOLT_ROUTING_UNABLE_TO_CONSTRUCT_POOL_FOR_SERVER   0x802
#define BOLT_ROUTING_UNABLE_TO_REFRESH_ROUTING_TABLE   0x803
#define BOLT_ROUTING_UNEXPECTED_DISCOVERY_RESPONSE   0x804
#define BOLT_CONNECTION_HAS_MORE_INFO   0xFFE
#define BOLT_STATUS_SET   0xFFF

PUBLIC const char* BoltError_get_string(int code);

#endif //SEABOLT_ALL_ERROR_H
