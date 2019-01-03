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

#ifndef SEABOLT_ALL_PROTOCOL_H
#define SEABOLT_ALL_PROTOCOL_H

#include "bolt-public.h"
#include "packstream.h"

#define FETCH_ERROR -1
#define FETCH_SUMMARY 0
#define FETCH_RECORD 1

struct BoltProtocol;

struct BoltConnection;

typedef int (* bool_func)(struct BoltConnection*);

typedef struct BoltValue* (* bolt_value_func)(struct BoltConnection*);

typedef char* (* char_func)(struct BoltConnection*);

typedef int16_t (* short_func)(struct BoltConnection*);

typedef const char* (* short_return_char_func)(int16_t);

typedef int (* init_func)(struct BoltConnection*, const char*, const struct BoltValue*);

typedef int (* goodbye_func)(struct BoltConnection*);

typedef int (* clear_begin_tx_func)(struct BoltConnection*);

typedef int (* set_begin_tx_bookmark_func)(struct BoltConnection*, struct BoltValue*);

typedef int (* set_begin_tx_metadata_func)(struct BoltConnection*, struct BoltValue*);

typedef int (* set_begin_tx_timeout_func)(struct BoltConnection*, int64_t);

typedef int (* load_begin_tx_func)(struct BoltConnection*);

typedef int (* load_commit_tx_func)(struct BoltConnection*);

typedef int (* load_rollback_tx_func)(struct BoltConnection*);

typedef int (* clear_run_func)(struct BoltConnection*);

typedef int (* set_run_bookmark_func)(struct BoltConnection*, struct BoltValue*);

typedef int (* set_run_tx_metadata_func)(struct BoltConnection*, struct BoltValue*);

typedef int (* set_run_tx_timeout_func)(struct BoltConnection*, int64_t);

typedef int (* set_run_cypher_func)(struct BoltConnection*, const char*, const size_t, int32_t);

typedef struct BoltValue* (* set_run_cypher_parameter_func)(struct BoltConnection*, int32_t, const char*, size_t);

typedef int (* load_run_func)(struct BoltConnection*);

typedef int (* load_discard_func)(struct BoltConnection*, int32_t);

typedef int (* load_pull_func)(struct BoltConnection*, int32_t);

typedef int (* load_reset_func)(struct BoltConnection*);

typedef BoltRequest (* last_request_func)(struct BoltConnection*);

typedef int (* fetch_func)(struct BoltConnection*, BoltRequest);

struct BoltProtocol {
    void* proto_state;

    short_return_char_func message_name;
    short_return_char_func structure_name;

    check_struct_signature_func check_readable_struct;
    check_struct_signature_func check_writable_struct;

    init_func init;
    goodbye_func goodbye;

    clear_begin_tx_func clear_begin_tx;
    set_begin_tx_bookmark_func set_begin_tx_bookmark;
    set_begin_tx_metadata_func set_begin_tx_metadata;
    set_begin_tx_timeout_func set_begin_tx_timeout;
    load_begin_tx_func load_begin_tx;

    load_commit_tx_func load_commit_tx;

    load_rollback_tx_func load_rollback_tx;

    clear_run_func clear_run;
    set_run_bookmark_func set_run_bookmark;
    set_run_tx_timeout_func set_run_tx_timeout;
    set_run_tx_metadata_func set_run_tx_metadata;
    set_run_cypher_func set_run_cypher;
    set_run_cypher_parameter_func set_run_cypher_parameter;
    load_run_func load_run;

    load_discard_func load_discard;
    load_pull_func load_pull;

    load_reset_func load_reset;

    last_request_func last_request;

    bolt_value_func field_names;
    bolt_value_func field_values;
    bolt_value_func metadata;
    bolt_value_func failure;

    bool_func is_success_summary;
    bool_func is_failure_summary;
    bool_func is_ignored_summary;

    short_func last_data_type;
    char_func last_bookmark;
    char_func server;
    char_func id;

    fetch_func fetch;
};

struct BoltMessage {
    int8_t code;
    struct BoltValue* fields;
};

struct BoltMessage* BoltMessage_create(int8_t code, int32_t n_fields);

void BoltMessage_destroy(struct BoltMessage* message);

struct BoltValue* BoltMessage_param(struct BoltMessage* message, int32_t index);

int
write_message(struct BoltMessage* message, check_struct_signature_func check_writable, struct BoltBuffer* buffer,
        const struct BoltLog* log);

void push_to_transmission(struct BoltBuffer* msg_buffer, struct BoltBuffer* tx_buffer);

#endif //SEABOLT_ALL_PROTOCOL_H
