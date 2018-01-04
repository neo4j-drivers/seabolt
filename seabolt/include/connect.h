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

/**
 * @file
 */

#ifndef SEABOLT_CONNECT
#define SEABOLT_CONNECT

#include <netdb.h>


#define try(code) { int status = (code); if (status == -1) { return status; } }


/**
 *
 */
enum BoltTransport
{
    BOLT_SECURE_SOCKET,
    BOLT_INSECURE_SOCKET,
};

/**
 *
 */
enum BoltConnectionStatus
{
    BOLT_DISCONNECTED,          // not connected
    BOLT_CONNECTED,             // connected but not authenticated
    BOLT_READY,                 // connected and authenticated
    BOLT_FAILED,                // recoverable failure
    BOLT_DEFUNCT,               // unrecoverable failure
};

/**
 *
 */
enum BoltConnectionError
{
    BOLT_NO_ERROR,
    BOLT_UNKNOWN_ERROR,
    BOLT_UNSUPPORTED,
    BOLT_INTERRUPTED,
    BOLT_TIMED_OUT,
    BOLT_PERMISSION_DENIED,
    BOLT_OUT_OF_FILES,
    BOLT_OUT_OF_MEMORY,
    BOLT_OUT_OF_PORTS,
    BOLT_CONNECTION_REFUSED,
    BOLT_NETWORK_UNREACHABLE,
    BOLT_TLS_ERROR,             // general catch-all for OpenSSL errors :/
    BOLT_PROTOCOL_VIOLATION,
    BOLT_END_OF_TRANSMISSION,
};


/**
 * Locator for a Bolt service.
 */
struct BoltService
{
    const char* host;
    unsigned short port;
    unsigned short n_resolved;
    struct
    {
        char host[16];
    } * resolved_data;
    int gai_status;
};



struct BoltService* BoltService_create(const char* host, unsigned short port);

void BoltService_resolve_b(struct BoltService * service);

void BoltService_destroy(struct BoltService * service);

void BoltService_write(struct BoltService * service, FILE * file);


/**
 * A Bolt client-server connection instance.
 *
 */
struct BoltConnection
{
    /// Transport type for this connection
    enum BoltTransport transport;

    /// The security context (secure connections only)
    struct ssl_ctx_st* ssl_context;
    /// A secure socket wrapper (secure connections only)
    struct ssl_st* ssl;
    /// The raw socket that backs this connection
    int socket;

    /// The protocol version used for this connection
    int32_t protocol_version;
    /// State required by the protocol
    void* protocol_state;

    // These buffers contain data exactly as it is transmitted or
    // received. Therefore for Bolt v1, chunk headers are included
    // in these buffers

    /// Transmit buffer
    struct BoltBuffer* tx_buffer;
    /// Receive buffer
    struct BoltBuffer* rx_buffer;

    /// Current status of the connection
    enum BoltConnectionStatus status;
    /// Current connection error code
    enum BoltConnectionError error;
};


/**
 * Open a connection to a Bolt server.
 *
 * This function allocates a new BoltConnection struct for the given _transport_
 * and attempts to connect it to _address_. The BoltConnection is returned
 * regardless of whether or not the connection attempt was successful.
 *
 * The address may point to any valid IPv4 or IPv6 address and will generally be
 * obtained via _getaddrinfo_.
 *
 * This function blocks until the connection attempt succeeds or fails.
 * On returning, the connection status will be set to either `BOLT_CONNECTED`
 * (if successful) or `BOLT_DEFUNCT` (if not). If defunct, the error code for
 * the connection will be set to one of the following:
 *
 * @verbatim embed:rst:leading-asterisk
 * ========================  ====================================================================
 * Error code                Description
 * ========================  ====================================================================
 * BOLT_CONNECTION_REFUSED   The remote server refused to accept the connection.
 * BOLT_INTERRUPTED          The connection attempt was interrupted.
 * BOLT_NETWORK_UNREACHABLE  The server address is on an unreachable network.
 * BOLT_OUT_OF_FILES         The system limit on the total number of open files has been reached.
 * BOLT_OUT_OF_MEMORY        Insufficient memory is available.
 * BOLT_OUT_OF_PORTS         No more local ports are available.
 * BOLT_PERMISSION_DENIED    The current process does not have permission to create a connection.
 * BOLT_TIMED_OUT            The connection attempt timed out.
 * BOLT_TLS_ERROR            An error occurred while attempting to secure the connection.
 * BOLT_UNKNOWN_ERROR        An error occurred for which no further detail can be determined.
 * BOLT_UNSUPPORTED          One or more connection parameters are unsupported.
 * ========================  ====================================================================
 * @endverbatim
 *
 * @param transport the type of transport over which to connect
 * @param address the address of the remote Bolt server
 * @return a pointer to a new BoltConnection struct
 */
struct BoltConnection* BoltConnection_open_b(enum BoltTransport transport, const struct addrinfo* address);

/**
 * Close a connection.
 *
 * @param connection
 */
void BoltConnection_close_b(struct BoltConnection* connection);

/**
 *
 * @param connection
 * @return
 */
struct BoltValue* BoltConnection_last_received(struct BoltConnection* connection);

/**
 * Initialise the connection and authenticate using the basic
 * authentication scheme.
 *
 * @param connection the connection to initialise
 * @param user_agent a string to identify this client software
 * @param user the user to authenticate as
 * @param password a valid password for the given user
 * @return
 */
int BoltConnection_init_b(struct BoltConnection* connection, const char* user_agent,
                          const char* user, const char* password);

/**
 * Transmit all queued requests.
 *
 * @param connection
 * @return
 */
int BoltConnection_transmit_b(struct BoltConnection* connection);

/**
 * Receive result values for all outstanding requests.
 *
 * @param connection
 * @return
 */
int BoltConnection_receive_b(struct BoltConnection* connection);

/**
 * Receive values from the current result stream, up to and including the
 * next summary.
 *
 * @param connection
 * @return
 */
int BoltConnection_receive_summary_b(struct BoltConnection* connection);

/**
 * Receive the next value from the current result stream.
 *
 * Each value will be either record data (stored in a BOLT_LIST) or
 * summary metadata (in a BOLT_SUMMARY). The value will be loaded into
 * `connection->received`.
 *
 * @param connection
 * @return 1 if record data is received, 0 if summary metadata is received
 *
 */
int BoltConnection_receive_value_b(struct BoltConnection* connection);

int BoltConnection_set_statement(struct BoltConnection* connection, const char* statement, size_t size);

int BoltConnection_resize_parameters(struct BoltConnection* connection, int32_t size);

int BoltConnection_set_parameter_key(struct BoltConnection* connection, int32_t index, const char* key, size_t key_size);

struct BoltValue* BoltConnection_parameter_value(struct BoltConnection* connection, int32_t index);

int BoltConnection_load_run(struct BoltConnection* connection);

int BoltConnection_load_discard(struct BoltConnection* connection, int32_t n);

int BoltConnection_load_pull(struct BoltConnection* connection, int32_t n);

#endif // SEABOLT_CONNECT
