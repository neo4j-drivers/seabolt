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

#include "bolt-private.h"
#include "address-private.h"
#include "config-private.h"
#include "connection-private.h"
#include "log-private.h"
#include "mem.h"
#include "protocol.h"
#include "time.h"
#include "v1.h"
#include "v2.h"
#include "v3.h"
#include "atomic.h"
#include "communication-plain.h"

#define INITIAL_TX_BUFFER_SIZE 8192
#define INITIAL_RX_BUFFER_SIZE 8192
#define ERROR_CTX_SIZE 1024

#define MAX_ID_LEN 32

#define TRY(code, error_ctx_fmt, file, line) { \
    int status_try = (code); \
    if (status_try != BOLT_SUCCESS) { \
        if (status_try == BOLT_STATUS_SET) { \
            return -1; \
        } else { \
            _set_status_with_ctx(connection, BOLT_CONNECTION_STATE_DEFUNCT, status_try, error_ctx_fmt, file, line, -1); \
        } \
        return status_try; \
    } \
}

static int64_t id_seq = 0;

void _status_changed(BoltConnection* connection)
{
    char* status_text = NULL;
    switch (connection->status->state) {
    case BOLT_CONNECTION_STATE_DISCONNECTED:
        status_text = "<DISCONNECTED>";
        break;
    case BOLT_CONNECTION_STATE_CONNECTED:
        status_text = "<CONNECTED>";
        break;
    case BOLT_CONNECTION_STATE_READY:
        status_text = "<READY>";
        break;
    case BOLT_CONNECTION_STATE_FAILED:
        status_text = "<FAILED>";
        break;
    case BOLT_CONNECTION_STATE_DEFUNCT:
        status_text = "<DEFUNCT>";
        break;
    }

    if (connection->status->error_ctx[0]!=0) {
        BoltLog_info(connection->log, "[%s]: %s [%s]", BoltConnection_id(connection), status_text,
                connection->status->error_ctx);
    }
    else {
        BoltLog_info(connection->log, "[%s]: %s", BoltConnection_id(connection), status_text);
    }

    if (connection->status->state==BOLT_CONNECTION_STATE_DEFUNCT
            || connection->status->state==BOLT_CONNECTION_STATE_FAILED) {
        if (connection->on_error_cb!=NULL) {
            (*connection->on_error_cb)(connection, connection->on_error_cb_state);
        }
    }
}

void _set_status(BoltConnection* connection, BoltConnectionState state, int error)
{
    BoltConnectionState old_status = connection->status->state;
    connection->status->state = state;
    connection->status->error = error;
    connection->status->error_ctx[0] = '\0';

    if (state!=old_status) {
        _status_changed(connection);
    }
}

void _set_status_with_ctx(BoltConnection* connection, BoltConnectionState status, int error,
        const char* error_ctx_format, ...)
{
    BoltConnectionState old_status = connection->status->state;
    connection->status->state = status;
    connection->status->error = error;
    connection->status->error_ctx[0] = '\0';
    if (error_ctx_format!=NULL) {
        va_list args;
        va_start(args, error_ctx_format);
        vsnprintf(connection->status->error_ctx, ERROR_CTX_SIZE, error_ctx_format, args);
        va_end(args);
    }

    if (status!=old_status) {
        _status_changed(connection);
    }
}

void _set_status_from_comm(BoltConnection* connection, BoltConnectionState state)
{
    _set_status_with_ctx(connection, state, connection->comm->status->error, connection->comm->status->error_ctx);
}

void _close(BoltConnection* connection)
{
    BoltLog_info(connection->log, "[%s]: Closing connection", BoltConnection_id(connection));

    if (connection->protocol!=NULL) {
        connection->protocol->goodbye(connection);

        switch (connection->protocol_version) {
        case 1:
            BoltProtocolV1_destroy_protocol(connection->protocol);
            break;
        case 2:
            BoltProtocolV2_destroy_protocol(connection->protocol);
            break;
        case 3:
            BoltProtocolV3_destroy_protocol(connection->protocol);
            break;
        default:
            break;
        }
        connection->protocol = NULL;
        connection->protocol_version = 0;
    }

    if (connection->comm!=NULL) {
        BoltCommunication_close(connection->comm, BoltConnection_id(connection));
        BoltCommunication_destroy(connection->comm);
        connection->comm = NULL;
    }

    BoltTime_get_time(&connection->metrics->time_closed);
    _set_status(connection, BOLT_CONNECTION_STATE_DISCONNECTED, BOLT_SUCCESS);
}

int handshake_b(BoltConnection* connection, int32_t _1, int32_t _2, int32_t _3, int32_t _4)
{
    BoltLog_info(connection->log, "[%s]: Performing handshake", BoltConnection_id(connection));
    char handshake[20];
    memcpy(&handshake[0x00], "\x60\x60\xB0\x17", 4);
    memcpy_be(&handshake[0x04], &_1, 4);
    memcpy_be(&handshake[0x08], &_2, 4);
    memcpy_be(&handshake[0x0C], &_3, 4);
    memcpy_be(&handshake[0x10], &_4, 4);
    int status = BoltCommunication_send(connection->comm, handshake, 20, BoltConnection_id(connection));
    if (status!=BOLT_SUCCESS) {
        _set_status_from_comm(connection, BOLT_CONNECTION_STATE_DEFUNCT);
        return BOLT_STATUS_SET;
    }
    int received = 0;
    status = BoltCommunication_receive(connection->comm, handshake, 4, 4, &received, BoltConnection_id(connection));
    if (status!=BOLT_SUCCESS) {
        _set_status_from_comm(connection, BOLT_CONNECTION_STATE_DEFUNCT);
        return BOLT_STATUS_SET;
    }
    memcpy_be(&connection->protocol_version, &handshake[0], 4);
    BoltLog_info(connection->log, "[%s]: <SET protocol_version=%d>", BoltConnection_id(connection),
            connection->protocol_version);
    switch (connection->protocol_version) {
    case 1:
        connection->protocol = BoltProtocolV1_create_protocol();
        return BOLT_SUCCESS;
    case 2:
        connection->protocol = BoltProtocolV2_create_protocol();
        return BOLT_SUCCESS;
    case 3:
        connection->protocol = BoltProtocolV3_create_protocol();
        return BOLT_SUCCESS;
    default:
        _close(connection);
        return BOLT_PROTOCOL_UNSUPPORTED;
    }
}

BoltConnection* BoltConnection_create()
{
    const size_t size = sizeof(BoltConnection);
    BoltConnection* connection = BoltMem_allocate(size);
    memset(connection, 0, size);
    connection->status = BoltStatus_create_with_ctx(ERROR_CTX_SIZE);
    connection->metrics = BoltMem_allocate(sizeof(BoltConnectionMetrics));
    memset(connection->metrics, 0, sizeof(BoltConnectionMetrics));
    return connection;
}

void BoltConnection_destroy(BoltConnection* connection)
{
    if (connection->status!=NULL) {
        BoltStatus_destroy(connection->status);
    }
    if (connection->metrics!=NULL) {
        BoltMem_deallocate(connection->metrics, sizeof(BoltConnectionMetrics));
    }
    BoltMem_deallocate(connection, sizeof(BoltConnection));
}

int32_t
BoltConnection_open(BoltConnection* connection, BoltTransport transport, struct BoltAddress* address,
        struct BoltTrust* trust, struct BoltLog* log, struct BoltSocketOptions* sock_opts)
{
    if (connection->status->state!=BOLT_CONNECTION_STATE_DISCONNECTED) {
        BoltConnection_close(connection);
    }
    // Id buffer composed of local&remote Endpoints
    connection->id = BoltMem_allocate(MAX_ID_LEN);
    snprintf(connection->id, MAX_ID_LEN, "conn-%" PRId64, BoltAtomic_increment(&id_seq));
    connection->log = log;
    // Store connection info
    connection->address = BoltAddress_create(address->host, address->port);

    switch (transport) {
    case BOLT_TRANSPORT_PLAINTEXT:
        connection->comm = BoltCommunication_create_plain(sock_opts, log);
        break;
    case BOLT_TRANSPORT_ENCRYPTED:
        connection->comm = BoltCommunication_create_secure(connection->sec_context, trust, sock_opts, log,
                connection->address->host, connection->id);
        break;
    }

    int status = BoltCommunication_open(connection->comm, address, connection->id);
    if (status==BOLT_SUCCESS) {
        BoltTime_get_time(&connection->metrics->time_opened);
        connection->tx_buffer = BoltBuffer_create(INITIAL_TX_BUFFER_SIZE);
        connection->rx_buffer = BoltBuffer_create(INITIAL_RX_BUFFER_SIZE);

        TRY(handshake_b(connection, 3, 2, 1, 0), "BoltConnection_open(%s:%d), handshake_b error code: %d", __FILE__,
                __LINE__);

        _set_status(connection, BOLT_CONNECTION_STATE_CONNECTED, BOLT_SUCCESS);
    }
    else {
        _set_status_from_comm(connection, BOLT_CONNECTION_STATE_DEFUNCT);
        return BOLT_STATUS_SET;
    }

    return connection->status->state==BOLT_CONNECTION_STATE_READY ? BOLT_SUCCESS : connection->status->error;
}

void BoltConnection_close(BoltConnection* connection)
{
    if (connection->status->state!=BOLT_CONNECTION_STATE_DISCONNECTED) {
        _close(connection);
    }
    if (connection->rx_buffer!=NULL) {
        BoltBuffer_destroy(connection->rx_buffer);
        connection->rx_buffer = NULL;
    }
    if (connection->tx_buffer!=NULL) {
        BoltBuffer_destroy(connection->tx_buffer);
        connection->tx_buffer = NULL;
    }
    if (connection->address!=NULL) {
        BoltAddress_destroy((struct BoltAddress*) connection->address);
        connection->address = NULL;
    }
    if (connection->id!=NULL) {
        BoltMem_deallocate(connection->id, MAX_ID_LEN);
        connection->id = NULL;
    }
}

int32_t BoltConnection_send(BoltConnection* connection)
{
    int size = BoltBuffer_unloadable(connection->tx_buffer);
    int status = BoltCommunication_send(connection->comm, BoltBuffer_unload_pointer(connection->tx_buffer, size), size,
            BoltConnection_id(connection));
    if (status!=BOLT_SUCCESS) {
        _set_status_from_comm(connection, BOLT_CONNECTION_STATE_DEFUNCT);
        status = BOLT_STATUS_SET;
    }
    BoltBuffer_compact(connection->tx_buffer);
    return status;
}

int BoltConnection_receive(BoltConnection* connection, char* buffer, int size)
{
    if (size==0) return 0;
    int available = BoltBuffer_unloadable(connection->rx_buffer);
    if (size>available) {
        int delta = size-available;
        while (delta>0) {
            int max_size = BoltBuffer_loadable(connection->rx_buffer);
            if (max_size==0) {
                BoltBuffer_compact(connection->rx_buffer);
                max_size = BoltBuffer_loadable(connection->rx_buffer);
            }
            max_size = delta>max_size ? delta : max_size;
            int received = 0;
            int status = BoltCommunication_receive(connection->comm,
                    BoltBuffer_load_pointer(connection->rx_buffer, max_size), delta, max_size,
                    &received, BoltConnection_id(connection));
            if (status!=BOLT_SUCCESS) {
                _set_status_from_comm(connection, BOLT_CONNECTION_STATE_DEFUNCT);
                return BOLT_STATUS_SET;
            }
            // adjust the buffer extent based on the actual amount of data received
            connection->rx_buffer->extent = connection->rx_buffer->extent-max_size+received;
            delta -= received;
        }
    }
    BoltBuffer_unload(connection->rx_buffer, buffer, size);
    return BOLT_SUCCESS;
}

int BoltConnection_fetch(BoltConnection* connection, BoltRequest request)
{
    const int fetched = connection->protocol->fetch(connection, request);
    if (fetched==FETCH_SUMMARY) {
        if (connection->protocol->is_success_summary(connection)) {
            _set_status(connection, BOLT_CONNECTION_STATE_READY, BOLT_SUCCESS);
        }
        else if (connection->protocol->is_ignored_summary(connection)) {
            // we may need to update status based on an earlier reported FAILURE
            // which our consumer did not care its result
            if (connection->protocol->failure(connection)!=NULL) {
                _set_status_with_ctx(connection, BOLT_CONNECTION_STATE_FAILED, BOLT_SERVER_FAILURE,
                        "BoltConnection_fetch(%s:%d), failure upon ignored message", __FILE__, __LINE__);
            }
        }
        else if (connection->protocol->is_failure_summary(connection)) {
            _set_status_with_ctx(connection, BOLT_CONNECTION_STATE_FAILED, BOLT_SERVER_FAILURE,
                    "BoltConnection_fetch(%s:%d), failure message", __FILE__, __LINE__);
        }
        else {
            BoltLog_error(connection->log, "[%s]: Protocol violation (received summary code %d)",
                    BoltConnection_id(connection), connection->protocol->last_data_type(connection));
            _set_status_with_ctx(connection, BOLT_CONNECTION_STATE_DEFUNCT, BOLT_PROTOCOL_VIOLATION,
                    "BoltConnection_fetch(%s:%d), received summary code: %d", __FILE__, __LINE__,
                    connection->protocol->last_data_type(connection));
            return FETCH_ERROR;
        }
    }
    return fetched;
}

int32_t BoltConnection_fetch_summary(BoltConnection* connection, BoltRequest request)
{
    int records = 0;
    int data;
    do {
        data = BoltConnection_fetch(connection, request);
        if (data<0) {
            return data;
        }
        records += data;
    }
    while (data);
    return records;
}

struct BoltValue* BoltConnection_field_values(BoltConnection* connection)
{
    return connection->protocol->field_values(connection);
}

int32_t BoltConnection_summary_success(BoltConnection* connection)
{
    return connection->protocol->is_success_summary(connection);
}

int32_t BoltConnection_summary_failure(BoltConnection* connection)
{
    return connection->protocol->is_failure_summary(connection);
}

int32_t
BoltConnection_init(BoltConnection* connection, const char* user_agent, const struct BoltValue* auth_token)
{
    BoltLog_info(connection->log, "[%s]: Initialising connection", BoltConnection_id(connection));
    switch (connection->protocol_version) {
    case 1:
    case 2:
    case 3: {
        int code = connection->protocol->init(connection, user_agent, auth_token);
        switch (code) {
        case BOLT_V1_SUCCESS:
            _set_status(connection, BOLT_CONNECTION_STATE_READY, BOLT_SUCCESS);
            return 0;
        case BOLT_V1_FAILURE:
            _set_status_with_ctx(connection, BOLT_CONNECTION_STATE_DEFUNCT, BOLT_PERMISSION_DENIED,
                    "BoltConnection_init(%s:%d), failure message", __FILE__, __LINE__);
            return -1;
        default:
            BoltLog_error(connection->log, "[%s]: Protocol violation (received summary code %d)",
                    BoltConnection_id(connection), code);
            _set_status_with_ctx(connection, BOLT_CONNECTION_STATE_DEFUNCT, BOLT_PROTOCOL_VIOLATION,
                    "BoltConnection_init(%s:%d), received summary code: %d", __FILE__, __LINE__, code);
            return -1;
        }
    }
    default:
        _set_status_with_ctx(connection, BOLT_CONNECTION_STATE_DEFUNCT, BOLT_PROTOCOL_UNSUPPORTED,
                "BoltConnection_init(%s:%d)",
                __FILE__, __LINE__);
        return -1;
    }
}

int32_t BoltConnection_clear_begin(BoltConnection* connection)
{
    TRY(connection->protocol->clear_begin_tx(connection), "BoltConnection_clear_begin(%s:%d), error code: %d", __FILE__,
            __LINE__);
    return BOLT_SUCCESS;
}

int32_t BoltConnection_set_begin_bookmarks(BoltConnection* connection, struct BoltValue* bookmark_list)
{
    TRY(connection->protocol->set_begin_tx_bookmark(connection, bookmark_list),
            "BoltConnection_set_begin_bookmarks(%s:%d), error code: %d", __FILE__, __LINE__);
    return BOLT_SUCCESS;
}

int32_t BoltConnection_set_begin_tx_timeout(BoltConnection* connection, int64_t timeout)
{
    TRY(connection->protocol->set_begin_tx_timeout(connection, timeout),
            "BoltConnection_set_begin_tx_timeout(%s:%d), error code: %d", __FILE__, __LINE__);
    return BOLT_SUCCESS;
}

int32_t BoltConnection_set_begin_tx_metadata(BoltConnection* connection, struct BoltValue* metadata)
{
    TRY(connection->protocol->set_begin_tx_metadata(connection, metadata),
            "BoltConnection_set_begin_tx_metadata(%s:%d), error code: %d", __FILE__, __LINE__);
    return BOLT_SUCCESS;
}

int32_t BoltConnection_load_begin_request(BoltConnection* connection)
{
    TRY(connection->protocol->load_begin_tx(connection), "BoltConnection_load_begin_request(%s:%d), error code: %d",
            __FILE__,
            __LINE__);
    return BOLT_SUCCESS;
}

int32_t BoltConnection_load_commit_request(BoltConnection* connection)
{
    TRY(connection->protocol->load_commit_tx(connection), "BoltConnection_load_commit_request(%s:%d), error code: %d",
            __FILE__,
            __LINE__);
    return BOLT_SUCCESS;
}

int32_t BoltConnection_load_rollback_request(BoltConnection* connection)
{
    TRY(connection->protocol->load_rollback_tx(connection),
            "BoltConnection_load_rollback_request(%s:%d), error code: %d",
            __FILE__, __LINE__);
    return BOLT_SUCCESS;
}

int32_t BoltConnection_clear_run(BoltConnection* connection)
{
    TRY(connection->protocol->clear_run(connection), "BoltConnection_clear_run(%s:%d), error code: %d", __FILE__,
            __LINE__);
    return BOLT_SUCCESS;
}

int32_t
BoltConnection_set_run_cypher(BoltConnection* connection, const char* cypher, const uint64_t cypher_size,
        const int32_t n_parameter)
{
    TRY(connection->protocol->set_run_cypher(connection, cypher, cypher_size, n_parameter),
            "BoltConnection_set_run_cypher(%s:%d), error code: %d", __FILE__, __LINE__);
    return BOLT_SUCCESS;
}

struct BoltValue*
BoltConnection_set_run_cypher_parameter(BoltConnection* connection, int32_t index, const char* name,
        const uint64_t name_size)
{
    return connection->protocol->set_run_cypher_parameter(connection, index, name, name_size);
}

int32_t BoltConnection_set_run_bookmarks(BoltConnection* connection, struct BoltValue* bookmark_list)
{
    TRY(connection->protocol->set_run_bookmark(connection, bookmark_list),
            "BoltConnection_set_run_bookmarks(%s:%d), error code: %d", __FILE__, __LINE__);
    return BOLT_SUCCESS;
}

int32_t BoltConnection_set_run_tx_timeout(BoltConnection* connection, int64_t timeout)
{
    TRY(connection->protocol->set_run_tx_timeout(connection, timeout),
            "BoltConnection_set_run_tx_timeout(%s:%d), error code: %d", __FILE__, __LINE__);
    return BOLT_SUCCESS;
}

int32_t BoltConnection_set_run_tx_metadata(BoltConnection* connection, struct BoltValue* metadata)
{
    TRY(connection->protocol->set_run_tx_metadata(connection, metadata),
            "BoltConnection_set_run_tx_metadata(%s:%d), error code: %d", __FILE__, __LINE__);
    return BOLT_SUCCESS;
}

int32_t BoltConnection_load_run_request(BoltConnection* connection)
{
    TRY(connection->protocol->load_run(connection), "BoltConnection_load_run_request(%s:%d), error code: %d", __FILE__,
            __LINE__);
    return BOLT_SUCCESS;
}

int32_t BoltConnection_load_discard_request(BoltConnection* connection, int32_t n)
{
    TRY(connection->protocol->load_discard(connection, n), "BoltConnection_load_discard_request(%s:%d), error code: %d",
            __FILE__, __LINE__);
    return BOLT_SUCCESS;
}

int32_t BoltConnection_load_pull_request(BoltConnection* connection, int32_t n)
{
    TRY(connection->protocol->load_pull(connection, n), "BoltConnection_load_pull_request(%s:%d), error code: %d",
            __FILE__, __LINE__);
    return BOLT_SUCCESS;
}

int32_t BoltConnection_load_reset_request(BoltConnection* connection)
{
    TRY(connection->protocol->load_reset(connection), "BoltConnection_load_reset_request(%s:%d), error code: %d",
            __FILE__, __LINE__);
    return BOLT_SUCCESS;
}

BoltRequest BoltConnection_last_request(BoltConnection* connection)
{
    return connection->protocol->last_request(connection);
}

const char* BoltConnection_server(BoltConnection* connection)
{
    return connection->protocol->server(connection);
}

const char* BoltConnection_id(BoltConnection* connection)
{
    if (connection->protocol!=NULL && connection->protocol->id!=NULL) {
        return connection->protocol->id(connection);
    }

    return connection->id;
}

const BoltAddress* BoltConnection_address(BoltConnection* connection)
{
    return connection->address;
}

const BoltAddress* BoltConnection_remote_endpoint(BoltConnection* connection)
{
    return connection->comm!=NULL ? BoltCommunication_remote_endpoint(connection->comm) : NULL;
}

const BoltAddress* BoltConnection_local_endpoint(BoltConnection* connection)
{
    return connection->comm!=NULL ? BoltCommunication_local_endpoint(connection->comm) : NULL;
}

const char* BoltConnection_last_bookmark(BoltConnection* connection)
{
    return connection->protocol->last_bookmark(connection);
}

struct BoltValue* BoltConnection_field_names(BoltConnection* connection)
{
    return connection->protocol->field_names(connection);
}

struct BoltValue* BoltConnection_metadata(BoltConnection* connection)
{
    return connection->protocol->metadata(connection);
}

struct BoltValue* BoltConnection_failure(BoltConnection* connection)
{
    return connection->protocol->failure(connection);
}

BoltStatus* BoltConnection_status(BoltConnection* connection)
{
    return connection->status;
}
