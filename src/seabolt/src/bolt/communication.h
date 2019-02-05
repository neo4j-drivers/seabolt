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
#ifndef SEABOLT_COMMUNICATION_H
#define SEABOLT_COMMUNICATION_H

#include "bolt-private.h"
#include "address.h"
#include "config.h"
#include "protocol.h"

typedef struct BoltCommunication BoltCommunication;

typedef BoltAddress* endpoint_func(struct BoltCommunication*);

typedef int int_func(int);

typedef int comm_func(BoltCommunication*);

typedef int comm_with_int_func(BoltCommunication*, int);

typedef int comm_open_func(BoltCommunication*, const struct sockaddr_storage*);

typedef int comm_send_func(BoltCommunication*, char*, int, int*);

typedef int comm_receive_func(BoltCommunication*, char*, int, int*);

typedef BoltAddress* comm_get_endpoint(BoltCommunication*);

typedef struct BoltCommunication {
    comm_open_func* open;
    comm_func* close;
    comm_send_func* send;
    comm_receive_func* recv;
    comm_func* destroy;

    comm_func* ignore_sigpipe;
    comm_func* restore_sigpipe;

    comm_func* last_error;
    comm_with_int_func* transform_error;

    comm_get_endpoint* get_local_endpoint;
    comm_get_endpoint* get_remote_endpoint;

    BoltStatus* status;
    int status_owned;
    BoltSocketOptions* sock_opts;
    int sock_opts_owned;
    BoltLog* log;
    void* context;
} BoltCommunication;

int BoltCommunication_startup();

int BoltCommunication_shutdown();

int BoltCommunication_open(BoltCommunication* comm, BoltAddress* address, const char* id);

int BoltCommunication_send(BoltCommunication* comm, char* buffer, int size, const char* id);

int BoltCommunication_close(BoltCommunication* comm, const char* id);

int BoltCommunication_receive(BoltCommunication* comm, char* buffer, int min_size, int max_size, int* received,
        const char* id);

BoltAddress* BoltCommunication_local_endpoint(BoltCommunication* comm);

BoltAddress* BoltCommunication_remote_endpoint(BoltCommunication* comm);

void BoltCommunication_destroy(BoltCommunication* comm);

#endif //SEABOLT_COMMUNICATION_H
