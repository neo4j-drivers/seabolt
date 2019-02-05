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
#ifndef SEABOLT_CONNECTION_PRIVATE_H
#define SEABOLT_CONNECTION_PRIVATE_H

#include "communication.h"
#include "communication-secure.h"
#include "connection.h"
#include "status-private.h"

typedef void (* error_action_func)(struct BoltConnection*, void*);

typedef struct BoltConnectionMetrics BoltConnectionMetrics;

/**
 * Record of connection usage statistics.
 */
struct BoltConnectionMetrics {
    struct timespec time_opened;
    struct timespec time_closed;
    unsigned long long bytes_sent;
    unsigned long long bytes_received;
};

struct BoltConnection {
    /// The agent currently responsible for using this connection
    const void* agent;

    /// Transport type for this connection
    BoltTransport transport;

    const BoltAddress* address;
    char* id;

    const BoltLog* log;

    /// The security context (secure connections only)
    BoltSecurityContext* sec_context;

    /// The communication object
    BoltCommunication* comm;

    /// The protocol version used for this connection
    int32_t protocol_version;
    /// State required by the protocol
    struct BoltProtocol* protocol;

    // These buffers contain data exactly as it is transmitted or
    // received. Therefore for Bolt v1, chunk headers are included
    // in these buffers

    /// Transmit buffer
    struct BoltBuffer* tx_buffer;
    /// Receive buffer
    struct BoltBuffer* rx_buffer;

    /// Connection metrics
    BoltConnectionMetrics* metrics;
    /// Current status of the connection
    BoltStatus* status;

    error_action_func on_error_cb;
    void* on_error_cb_state;
};

/**
 * Take an exact amount of data from the receive buffer, deferring to
 * the socket if not enough data is available.
 *
 * @param connection
 * @param buffer
 * @param buffer_size
 * @return
 */
int32_t BoltConnection_receive(BoltConnection* connection, char* buffer, int buffer_size);

#endif //SEABOLT_CONNECTION_PRIVATE_H
