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
 *  The type that represents a Bolt client-server connection.
 *
 *
 */
typedef struct BoltConnection BoltConnection;


/**
 * Sends all of the queued requests.
 *
 * @param connection the instance for which send will be carried out.
 * @return \ref BOLT_SUCCESS on success or an error code in case of a failure.
 */
SEABOLT_EXPORT int32_t BoltConnection_send(BoltConnection* connection);

/**
 * Fetches the next value from the result stream for a given request.
 * This will discard the responses of earlier requests that have not
 * already been fully consumed. This function will always consume at
 * least one record from the result stream and is not able to check
 * whether the given request has already been fully consumed; doing
 * so is the responsibility of the calling application.
 *
 * The function returns 1 if a record data is received and corresponding data
 * is available through \ref BoltConnection_field_values function.
 *
 * The function returns 0 if a summary metadata is received and corresponding
 * data is available through \ref BoltConnection_metadata and
 * \ref BoltConnection_field_names (if this is for a RUN request) functions.
 *
 * The function returns -1 if an error occurs, and more information about the
 * underlying error can be gathered through \ref BoltStatus for which you can
 * get a reference by calling \ref BoltConnection_status.
 *
 * This function will block until an appropriate value has been fetched.
 *
 * @param connection the instance to fetch from
 * @param request the request for which to fetch a response
 * @return 1 if record data is received,
 *         0 if summary metadata is received,
 *         -1 if an error occurs
 *
 */
SEABOLT_EXPORT int32_t BoltConnection_fetch(BoltConnection* connection, BoltRequest request);

/**
 * Fetch values from the result stream for a given request, up to and
 * including the next summary. This will discard any unconsumed result
 * data for this request as well as the responses of earlier requests
 * that have not already been fully consumed. This function will always
 * consume at least one record from the result stream and is not able
 * to check whether the given request has already been fully consumed;
 * doing so is the responsibility of the calling application.
 *
 * The function returns >=0 if the call succeeds and corresponding
 * data is available through \ref BoltConnection_metadata and
 * \ref BoltConnection_field_names (if this is for a RUN request) functions.
 *
 * The function returns -1 if an error occurs, and more information about the
 * underlying error can be gathered through \ref BoltStatus for which you can
 * get a reference by calling \ref BoltConnection_status.
 *
 * This function will block until an appropriate value has been fetched.
 *
 * @param connection the instance to fetch from
 * @param request the request for which to fetch a response
 * @return >=0 the number of records discarded from this result
 *         -1 if an error occurs
 */
SEABOLT_EXPORT int32_t BoltConnection_fetch_summary(BoltConnection* connection, BoltRequest request);

/**
 * Clears the buffered BEGIN TRANSACTION message.
 *
 * @param connection the instance on which to clear buffered BEGIN TRANSACTION message.
 * @return \ref BOLT_SUCCESS on success,
 *          an error code on failure.
 */
SEABOLT_EXPORT int32_t BoltConnection_clear_begin(BoltConnection* connection);

/**
 * Sets bookmark list on the buffered BEGIN TRANSACTION message.
 *
 * @param connection the instance on which to update buffered BEGIN TRANSACTION message.
 * @param bookmark_list the list of bookmarks to set.
 * @return \ref BOLT_SUCCESS on success,
 *          an error code on failure.
 */
SEABOLT_EXPORT int32_t BoltConnection_set_begin_bookmarks(BoltConnection* connection, BoltValue* bookmark_list);

/**
 * Sets transaction timeout on the buffered BEGIN TRANSACTION message.
 *
 * @param connection the instance on which to update buffered BEGIN TRANSACTION message
 * @param timeout the timeout value to apply.
 * @return \ref BOLT_SUCCESS on success,
 *          an error code on failure.
 */
SEABOLT_EXPORT int32_t BoltConnection_set_begin_tx_timeout(BoltConnection* connection, int64_t timeout);

/**
 * Sets transaction metadata on the buffered BEGIN TRANSACTION message.
 *
 * @param connection the instance on which to update buffered BEGIN TRANSACTION message
 * @param metadata the metadata dictionary value to apply.
 * @return \ref BOLT_SUCCESS on success,
 *          an error code on failure.
 */
SEABOLT_EXPORT int32_t BoltConnection_set_begin_tx_metadata(BoltConnection* connection, BoltValue* metadata);

/**
 * Loads the buffered BEGIN TRANSACTION message into the request queue.
 *
 * @param connection the instance on which to queue the message.
 * @return \ref BOLT_SUCCESS on success,
 *          an error code on failure.
 */
SEABOLT_EXPORT int32_t BoltConnection_load_begin_request(BoltConnection* connection);

/**
 * Loads a COMMIT TRANSACTION message into the request queue.
 *
 * @param connection the instance to queue the request into.
 * @return \ref BOLT_SUCCESS on success,
 *          an error code on failure.
 */
SEABOLT_EXPORT int32_t BoltConnection_load_commit_request(BoltConnection* connection);

/**
 * Loads a ROLLBACK TRANSACTION message into the request queue.
 *
 * @param connection the instance to queue the request into.
 * @return \ref BOLT_SUCCESS on success,
 *          an error code on failure.
 */
SEABOLT_EXPORT int32_t BoltConnection_load_rollback_request(BoltConnection* connection);

/**
 * Clears the buffered RUN message.
 *
 * @param connection the instance on which to clear buffered RUN message.
 * @return \ref BOLT_SUCCESS on success,
 *          an error code on failure.
 */
SEABOLT_EXPORT int32_t BoltConnection_clear_run(BoltConnection* connection);

/**
 * Sets bookmark list on the buffered RUN message.
 *
 * @param connection the instance on which to update buffered RUN message.
 * @param bookmark_list the list of bookmarks to set.
 * @return \ref BOLT_SUCCESS on success,
 *          an error code on failure.
 */
SEABOLT_EXPORT int32_t BoltConnection_set_run_bookmarks(BoltConnection* connection, BoltValue* bookmark_list);

/**
 * Sets transaction timeout on the buffered RUN message.
 *
 * @param connection the instance on which to update buffered RUN message
 * @param timeout the timeout value to apply.
 * @return \ref BOLT_SUCCESS on success,
 *          an error code on failure.
 */
SEABOLT_EXPORT int32_t BoltConnection_set_run_tx_timeout(BoltConnection* connection, int64_t timeout);

/**
 * Sets transaction metadata on the buffered RUN message.
 *
 * @param connection the instance on which to update buffered RUN message
 * @param metadata the metadata dictionary value to apply.
 * @return \ref BOLT_SUCCESS on success,
 *          an error code on failure.
 */
SEABOLT_EXPORT int32_t BoltConnection_set_run_tx_metadata(BoltConnection* connection, BoltValue* metadata);

/**
 * Sets the cypher query on the buffered RUN message.
 *
 * @param connection the instance on which to update buffered RUN message.
 * @param cypher the cypher text to execute.
 * @param cypher_size the size of the cypher text.
 * @param n_parameter number of cypher parameters to be allocated on the message.
 * @return \ref BOLT_SUCCESS on success,
 *          an error code on failure.
 */
SEABOLT_EXPORT int32_t
BoltConnection_set_run_cypher(BoltConnection* connection, const char* cypher, const uint64_t cypher_size,
        const int32_t n_parameter);

/**
 * Sets the cypher parameter's value on the buffered RUN message.
 *
 * @param connection the instance on which to update buffered RUN message.
 * @param index the index of the parameter to update.
 * @param name the parameter name to set.
 * @param name_size size of the parameter name.
 * @return a reference to an already allocated \ref BoltValue to be populated.
 */
SEABOLT_EXPORT BoltValue*
BoltConnection_set_run_cypher_parameter(BoltConnection* connection, int32_t index, const char* name,
        const uint64_t name_size);

/**
 * Loads the buffered RUN message into the request queue.
 *
 * @param connection the instance on which to queue the message.
 * @return \ref BOLT_SUCCESS on success,
 *          an error code on failure.
 */
SEABOLT_EXPORT int32_t BoltConnection_load_run_request(BoltConnection* connection);

/**
 * Loads a DISCARD_ALL message into the request queue.
 *
 * @param connection the instance to queue the request into.
 * @param n not used currently, should be set to -1.
 * @return \ref BOLT_SUCCESS on success,
 *          an error code on failure.
 */
SEABOLT_EXPORT int32_t BoltConnection_load_discard_request(BoltConnection* connection, int32_t n);

/**
 * Loads a PULL_ALL message into the request queue.
 *
 * @param connection the instance to queue the request into.
 * @param n not used currently, should be set to -1.
 * @return \ref BOLT_SUCCESS on success,
 *          an error code on failure.
 */
SEABOLT_EXPORT int32_t BoltConnection_load_pull_request(BoltConnection* connection, int32_t n);

/**
 * Loads a RESET message into the request queue.
 *
 * RESET message resets the connection to discard any outstanding results,
 * rollback the current transaction and clear any unacknowledged
 * failures.
 *
 * @param connection the instance to queue the request into.
 * @return \ref BOLT_SUCCESS on success,
 *          an error code on failure.
 */
SEABOLT_EXPORT int32_t BoltConnection_load_reset_request(BoltConnection* connection);

/**
 * Returns a handle to the last request queued to be sent to the server. This handle
 * can be used to fetch response data for a particular request.
 *
 * @param connection the instance to query.
 * @returns the last queued message's \ref BoltRequest "handle".
 */
SEABOLT_EXPORT BoltRequest BoltConnection_last_request(BoltConnection* connection);

/**
 * Returns the server identification (vendor/version) string as presented by the server.
 *
 * @param connection the instance to query.
 * @returns the server identification string if available, NULL otherwise.
 */
SEABOLT_EXPORT const char* BoltConnection_server(BoltConnection* connection);

/**
 * Returns a unique connection identifier for this connection.
 *
 * The returned string is unique across the process, and also is appended with any connection identifier
 * assigned by the server (only available with Bolt V3). The appended connection identifier can
 * be used to form a correlation between client and server side information (such as active connections,
 * active transactions, active queries, etc.).
 *
 * @param connection the instance to query.
 * @returns the identifier string.
 */
SEABOLT_EXPORT const char* BoltConnection_id(BoltConnection* connection);

/**
 * Returns the server address as specified to \ref BoltConnection_open.
 *
 * @param connection the instance to query.
 * @returns the server address.
 */
SEABOLT_EXPORT const BoltAddress* BoltConnection_address(BoltConnection* connection);

/**
 * Returns the remote endpoint (IP address / port) of the active connection.
 *
 * @param connection the instance to query.
 * @returns the remote endpoint.
 */
SEABOLT_EXPORT const BoltAddress* BoltConnection_remote_endpoint(BoltConnection* connection);

/**
 * Returns the local endpoint (IP address / port) of the active connection.
 *
 * @param connection the instance to query.
 * @returns the local endpoint.
 */
SEABOLT_EXPORT const BoltAddress* BoltConnection_local_endpoint(BoltConnection* connection);

/**
 * Returns the latest bookmark sent by the server.
 *
 * This may return null if server did not return any bookmark data for this connection.
 * This pointer is alive, which means the underlying bookmark data may be changed over
 * time with updated data on this same connection. Do not change underlying data and
 * clone it if you want to have a fixed bookmark in-hand.
 *
 * @param connection the instance to query.
 * @returns the latest bookmark received.
 */
SEABOLT_EXPORT const char* BoltConnection_last_bookmark(BoltConnection* connection);

/**
 * Checks whether the last received data is a SUMMARY message of SUCCESS.
 *
 * @param connection the instance to query.
 * @returns 1 if the last received data is a SUCCESS summary message,
 *          0 otherwise
 */
SEABOLT_EXPORT int32_t BoltConnection_summary_success(BoltConnection* connection);

/**
 * Returns the details of the latest server generated FAILURE message
 *
 * The returned \ref BoltValue is a dictionary with _code_ and _message_ keys, each
 * pointing to a string value identifying the code and message of the failure
 * server generated.
 *
 * @param connection the instance to query.
 * @return a \ref BoltValue "dictionary" representing the FAILURE data.
 */
SEABOLT_EXPORT BoltValue* BoltConnection_failure(BoltConnection* connection);

/**
 * Returns the field names available in the current result.
 *
 * The returned \ref BoltValue is a list of field names, each pointing to a string value
 * identifying the corresponding field name.
 *
 * @param connection the instance to query.
 * @return a \ref BoltValue "list" containing the field names.
 */
SEABOLT_EXPORT BoltValue* BoltConnection_field_names(BoltConnection* connection);

/**
 * Returns the field values available in the last received record.
 *
 * The returned \ref BoltValue is a list of field values, each pointing to an arbitrary value
 * as generated by the executed cypher query.
 *
 * @param connection the instance to query.
 * @return a \ref BoltValue "list" containing the field values.
 */
SEABOLT_EXPORT BoltValue* BoltConnection_field_values(BoltConnection* connection);

/**
 * Returns the metadata fields sent by the server in the last SUMMARY message.
 *
 * The returned \ref BoltValue is a dictionary with arbitrary keys, each
 * pointing to a corresponding value.
 *
 * @param connection the instance to query.
 * @return a \ref BoltValue "dictionary" representing the metadata fields.
 */
SEABOLT_EXPORT BoltValue* BoltConnection_metadata(BoltConnection* connection);

/**
 * Returns the current status data of the connection.
 *
 * @param connection the instance to query.
 * @return a \ref BoltStatus instance containing further status information.
 */
SEABOLT_EXPORT BoltStatus* BoltConnection_status(BoltConnection* connection);

#endif // SEABOLT_CONNECTION
