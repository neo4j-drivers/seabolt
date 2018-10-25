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

/**
 * @file
 */

#ifndef SEABOLT_CONNECTION
#define SEABOLT_CONNECTION

#include "bolt-public.h"
#include "address.h"
#include "config.h"
#include "status.h"

typedef uint64_t BoltRequest;

/**
 * A Bolt client-server connection instance.
 *
 */
typedef struct BoltConnection BoltConnection;

/**
 * Create a new connection.
 *
 * @return
 */
SEABOLT_EXPORT BoltConnection* BoltConnection_create();

/**
 * Destroy a connection.
 */
SEABOLT_EXPORT void BoltConnection_destroy(BoltConnection* connection);

/**
 * Open a connection to a Bolt server.
 *
 * This function attempts to connect a BoltConnection to _address_ over
 * _transport_. The `address` should be a pointer to a `BoltAddress` struct
 * that has been successfully resolved.
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
 * BOLT_UNRESOLVED_ADDRESS   The supplied address has not been resolved.
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
 * @param connection the connection to open
 * @param transport the type of transport over which to connect
 * @param address descriptor of the remote Bolt server address
 * @return 0 if the connection was opened successfully, -1 otherwise
 */
SEABOLT_EXPORT int BoltConnection_open(BoltConnection* connection, BoltTransport transport,
        BoltAddress* address, struct BoltTrust* trust, struct BoltLog* log, struct BoltSocketOptions* sock_opts);

/**
 * Close a connection.
 *
 * @param connection
 */
SEABOLT_EXPORT void BoltConnection_close(BoltConnection* connection);

/**
 * Initialise the connection and authenticate using the basic
 * authentication scheme.
 *
 * @param connection the connection to initialise
 * @param user_agent the user-agent string
 * @param auth_token dictionary that contains user credentials
 * @return
 */
SEABOLT_EXPORT int
BoltConnection_init(BoltConnection* connection, const char* user_agent, const BoltValue* auth_token);

/**
 * Send all queued requests.
 *
 * @param connection
 * @return the latest request ID
 */
SEABOLT_EXPORT int BoltConnection_send(BoltConnection* connection);

/**
 * Take an exact amount of data from the receive buffer, deferring to
 * the socket if not enough data is available.
 *
 * @param connection
 * @param buffer
 * @param size
 * @return
 */
int BoltConnection_receive(BoltConnection* connection, char* buffer, int size);

/**
 * Fetch the next value from the result stream for a given request.
 * This will discard the responses of earlier requests that have not
 * already been fully consumed. This function will always consume at
 * least one record from the result stream and is not able to check
 * whether the given request has already been fully consumed; doing
 * so is the responsibility of the calling application.
 *
 * After calling this function, the value returned by
 * `BoltConnection_data` should contain either record data
 * (stored in a `BOLT_LIST`) or summary metadata (in a `BOLT_SUMMARY`).
 *
 * This function will block until an appropriate value has been fetched.
 *
 * @param connection the connection to fetch from
 * @param request the request for which to fetch a response
 * @return 1 if record data is received,
 *         0 if summary metadata is received,
 *         -1 if an error occurs
 *
 */
SEABOLT_EXPORT int BoltConnection_fetch(BoltConnection* connection, BoltRequest request);

/**
 * Fetch values from the result stream for a given request, up to and
 * including the next summary. This will discard any unconsumed result
 * data for this request as well as the responses of earlier requests
 * that have not already been fully consumed. This function will always
 * consume at least one record from the result stream and is not able
 * to check whether the given request has already been fully consumed;
 * doing so is the responsibility of the calling application.
 *
 * After calling this function, the value returned by
 * `BoltConnection_data` should contain the summary metadata of the
 * received result (in a `BOLT_SUMMARY`).
 *
 * @param connection the connection to fetch from
 * @param request the request for which to fetch a response
 * @return >=0 the number of records discarded from this result
 *         -1 if an error occurs
 */
SEABOLT_EXPORT int BoltConnection_fetch_summary(BoltConnection* connection, BoltRequest request);

/**
 * Load a transaction BEGIN request into the request queue.
 *
 * @param connection
 * @return
 */
SEABOLT_EXPORT int BoltConnection_clear_begin(BoltConnection* connection);

SEABOLT_EXPORT int BoltConnection_set_begin_bookmarks(BoltConnection* connection, BoltValue* bookmark_list);

SEABOLT_EXPORT int BoltConnection_set_begin_tx_timeout(BoltConnection* connection, int64_t timeout);

SEABOLT_EXPORT int BoltConnection_set_begin_tx_metadata(BoltConnection* connection, BoltValue* metadata);

/**
 * Load a transaction BEGIN request into the request queue.
 *
 * @param connection
 * @return
 */
SEABOLT_EXPORT int BoltConnection_load_begin_request(BoltConnection* connection);

/**
 * Load a transaction COMMIT request into the request queue.
 *
 * @param connection
 * @return
 */
SEABOLT_EXPORT int BoltConnection_load_commit_request(BoltConnection* connection);

/**
 * Load a transaction ROLLBACK request into the request queue.
 *
 * @param connection
 * @return
 */
SEABOLT_EXPORT int BoltConnection_load_rollback_request(BoltConnection* connection);

/**
 * Load a RUN request into the request queue.
 *
 * @param connection
 * @return
 */
SEABOLT_EXPORT int BoltConnection_clear_run(BoltConnection* connection);

SEABOLT_EXPORT int BoltConnection_set_run_bookmarks(BoltConnection* connection, BoltValue* bookmark_list);

SEABOLT_EXPORT int BoltConnection_set_run_tx_timeout(BoltConnection* connection, int64_t timeout);

SEABOLT_EXPORT int BoltConnection_set_run_tx_metadata(BoltConnection* connection, BoltValue* metadata);

SEABOLT_EXPORT int
BoltConnection_set_run_cypher(BoltConnection* connection, const char* cypher, const size_t cypher_size,
        int32_t n_parameter);

SEABOLT_EXPORT BoltValue*
BoltConnection_set_run_cypher_parameter(BoltConnection* connection, int32_t index, const char* name,
        size_t name_size);

SEABOLT_EXPORT int BoltConnection_load_run_request(BoltConnection* connection);

/**
 * Load a DISCARD_ALL request into the request queue.
 *
 * @param connection
 * @param n should always be -1
 * @return
 */
SEABOLT_EXPORT int BoltConnection_load_discard_request(BoltConnection* connection, int32_t n);

/**
 * Load a PULL_ALL request into the request queue.
 *
 * @param connection
 * @param n should always be -1
 * @return
 */
SEABOLT_EXPORT int BoltConnection_load_pull_request(BoltConnection* connection, int32_t n);

/**
 * Load a RESET request into the request queue.
 *
 * RESET message resets the connection to discard any outstanding results,
 * rollback the current transaction and clear any unacknowledged
 * failures.
 *
 * @param connection
 * @return
 */
SEABOLT_EXPORT int BoltConnection_load_reset_request(BoltConnection* connection);

/**
 * Obtain a handle to the last request sent to the server. This handle
 * can be used to fetch response data for a particular request.
 *
 * @param connection
 * @return
 */
SEABOLT_EXPORT BoltRequest BoltConnection_last_request(BoltConnection* connection);

SEABOLT_EXPORT char* BoltConnection_server(BoltConnection* connection);

SEABOLT_EXPORT char* BoltConnection_id(BoltConnection* connection);

SEABOLT_EXPORT const BoltAddress* BoltConnection_address(BoltConnection* connection);

SEABOLT_EXPORT const BoltAddress* BoltConnection_remote_endpoint(BoltConnection* connection);

SEABOLT_EXPORT const BoltAddress* BoltConnection_local_endpoint(BoltConnection* connection);

/**
 * Obtain the latest bookmark sent by the server. This may return null if
 * server did not return any bookmark data for this connection. This pointer is
 * alive, which means the underlying bookmark data may be changed over time with
 * updated data on this same connection. Do not change underlying data and clone
 * it if you want to have a fixed bookmark in-hand.
 *
 * @param connection
 * @return
 */
SEABOLT_EXPORT char* BoltConnection_last_bookmark(BoltConnection* connection);


/**
*
* @param connection
* @return
*/
SEABOLT_EXPORT int BoltConnection_summary_success(BoltConnection* connection);

/**
 * Obtain the details of the latest server generated FAILURE message
 *
 * @param connection
 * @return
 */
SEABOLT_EXPORT BoltValue* BoltConnection_failure(BoltConnection* connection);

/**
 * Return the fields available in the current result.
 *
 * @param connection
 * @return
 */
SEABOLT_EXPORT BoltValue* BoltConnection_field_names(BoltConnection* connection);

/**
* Obtain a value from the current record.
*
* @param connection
* @param field
* @return pointer to a `BoltValue` data structure formatted as a BOLT_LIST
*/
SEABOLT_EXPORT BoltValue* BoltConnection_field_values(BoltConnection* connection);

/**
 * Returns the metadata sent by the server.
 *
 * @param connection
 * @return
 */
SEABOLT_EXPORT BoltValue* BoltConnection_metadata(BoltConnection* connection);

SEABOLT_EXPORT BoltStatus* BoltConnection_status(BoltConnection* connection);

#endif // SEABOLT_CONNECTION
