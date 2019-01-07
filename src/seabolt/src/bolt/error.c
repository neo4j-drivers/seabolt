/*
 * Copyright (c) 2002-2019 "Neo4j,"
 * Neo4j Sweden AB [http://neo4jcom]
 *
 * This file is part of Neo4j
 *
 * Licensed under the Apache License, Version 20 (the "License");
 * you may not use this file except in compliance with the License
 * You may obtain a copy of the License at
 *
 *     http://wwwapacheorg/licenses/LICENSE-20
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied
 * See the License for the specific language governing permissions and
 * limitations under the License
 */

#include "bolt-private.h"

const char* BoltError_get_string(int32_t code)
{
    switch (code) {
    case BOLT_SUCCESS:
        return "operation succeeded";
    case BOLT_UNKNOWN_ERROR:
        return "an unknown error occurred";
    case BOLT_UNSUPPORTED:
        return "an unsupported protocol or address family";
    case BOLT_INTERRUPTED:
        return "a blocking operation interrupted";
    case BOLT_CONNECTION_RESET:
        return "connection reset";
    case BOLT_NO_VALID_ADDRESS:
        return "no valid resolved addresses found to connect";
    case BOLT_TIMED_OUT:
        return "an operation timed out";
    case BOLT_PERMISSION_DENIED:
        return "permission denied";
    case BOLT_OUT_OF_FILES:
        return "too may open files";
    case BOLT_OUT_OF_MEMORY:
        return "out of memory";
    case BOLT_OUT_OF_PORTS:
        return "too many open ports";
    case BOLT_CONNECTION_REFUSED:
        return "connection refused";
    case BOLT_NETWORK_UNREACHABLE:
        return "network unreachable";
    case BOLT_TLS_ERROR:
        return "an unknown TLS error";
    case BOLT_END_OF_TRANSMISSION:
        return "connection closed by remote peer";
    case BOLT_SERVER_FAILURE:
        return "neo4j failure";
    case BOLT_TRANSPORT_UNSUPPORTED:
        return "unsupported bolt transport";
    case BOLT_PROTOCOL_VIOLATION:
        return "unsupported protocol usage";
    case BOLT_PROTOCOL_UNSUPPORTED_TYPE:
        return "unsupported bolt type";
    case BOLT_PROTOCOL_NOT_IMPLEMENTED_TYPE:
        return "unknown pack stream type";
    case BOLT_PROTOCOL_UNEXPECTED_MARKER:
        return "unexpected marker";
    case BOLT_PROTOCOL_UNSUPPORTED:
        return "unsupported bolt protocol version";
    case BOLT_POOL_FULL:
        return "connection pool full";
    case BOLT_POOL_ACQUISITION_TIMED_OUT:
        return "connection acquisition from the connection pool timed out";
    case BOLT_ADDRESS_NOT_RESOLVED:
        return "address resolution failed";
    case BOLT_ROUTING_UNABLE_TO_RETRIEVE_ROUTING_TABLE:
        return "routing table retrieval failed";
    case BOLT_ROUTING_NO_SERVERS_TO_SELECT:
        return "no servers to select for the requested operation";
    case BOLT_ROUTING_UNABLE_TO_CONSTRUCT_POOL_FOR_SERVER:
        return "connection pool construction for server failed";
    case BOLT_ROUTING_UNABLE_TO_REFRESH_ROUTING_TABLE:
        return "routing table refresh failed";
    case BOLT_ROUTING_UNEXPECTED_DISCOVERY_RESPONSE:
        return "invalid discovery response";
    case BOLT_CONNECTION_HAS_MORE_INFO:
        return "error set in connection";
    case BOLT_STATUS_SET:
        return "error set in connection";
    default:
        return "UNKNOWN ERROR CODE";
    }
}

