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
#ifndef SEABOLT_BOLT_PRIVATE_H
#define SEABOLT_BOLT_PRIVATE_H

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#if USE_POSIXSOCK

#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <fcntl.h>
#include <netdb.h>
#include <signal.h>

#endif // USE_POSIXSOCK

#if USE_WINSOCK

#include <winsock2.h>
#include <windows.h>
#include <Ws2tcpip.h>

#endif // USE_WINSOCK

#include "error.h"

#define SIZE_OF_C_STRING(str) (sizeof(char)*(strlen(str)+1))
#define UNUSED(x) (void)(x)

#endif //SEABOLT_BOLT_PRIVATE_H
