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
#include "connection.h"
#include "status-private.h"
#include "bolt.h"

/**
 * Use BOLT_TRANSPORT_MOCK to establish a mock connection
 */
#define BOLT_TRANSPORT_MOCKED    -1

typedef void (* error_action_func)(struct BoltConnection*, void*);

typedef struct BoltConnectionMetrics BoltConnectionMetrics;

typedef struct BoltSecurityContext BoltSecurityContext;

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
    BoltAccessMode access_mode;

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
 * Creates a new instance of \ref BoltConnection.
 *
 * @return the pointer to the newly allocated \ref BoltConnection instance.
 */
BoltConnection* BoltConnection_create();

/**
 * Destroys the passed \ref BoltConnection instance.
 *
 * @param connection the instance to be destroyed.
 */
void BoltConnection_destroy(BoltConnection* connection);

/**
 * Opens a connection to a Bolt server.
 *
 * This function attempts to connect a BoltConnection to _address_ over
 * _transport_. The `address` should be a pointer to a `BoltAddress` struct
 * that has been successfully resolved.
 *
 * This function blocks until the connection attempt succeeds or fails.
 * On returning, the connection status will be set to either \ref BOLT_CONNECTION_STATE_CONNECTED
 * (if successful) or \ref BOLT_CONNECTION_STATE_DEFUNCT (if not). If defunct, the actual error code will
 * be returned.
 *
 * In case an error code is returned, more information can be gathered through \ref BoltStatus for which you can
 * get a reference by calling \ref BoltConnection_status.
 *
 * @param connection the instance to attempt the connection.
 * @param transport the type of transport over which to connect.
 * @param address the Bolt server address.
 * @param trust the trust settings to be used for \ref BOLT_TRANSPORT_ENCRYPTED connections.
 * @param log the logger to be used for logging purposes.
 * @param sock_opts the socket options to be applied to the underlying socket.
 * @return \ref BOLT_SUCCESS if the connection was opened successfully,
 *          an error code otherwise.
 */
int32_t BoltConnection_open(BoltConnection* connection, BoltTransport transport,
        BoltAddress* address, BoltTrust* trust, BoltLog* log, BoltSocketOptions* sock_opts);

/**
 * Closes the connection.
 *
 * @param connection the instance to be closed.
 */
void BoltConnection_close(BoltConnection* connection);

/**
 * Initialise the connection and authenticate using the provided authentication token.
 *
 * Returns 0 on success and -1 in case of an error. More information about the underlying error can be gathered
 * through \ref BoltStatus for which you can get a reference by calling \ref BoltConnection_status.
 *
 * @param connection the instance to be initialised and authenticated.
 * @param user_agent the user agent string to present to the server.
 * @param auth_token dictionary that contains an authentication token.
 * @return 0 on success,
 *        -1 on error.
 */
int32_t
BoltConnection_init(BoltConnection* connection, const char* user_agent, const BoltValue* auth_token);

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
