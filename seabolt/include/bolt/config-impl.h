/*
* Copyright (c) 2002-2018 "Neo Technology,"
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
#ifndef SEABOLT_CONFIG_IMPL
#define SEABOLT_CONFIG_IMPL
#include "config.h"

#ifdef _WIN32
#pragma  warning(push)
#pragma  warning(disable:4255)
#pragma  warning(disable:4265)
#pragma  warning(disable:4625)
#pragma  warning(disable:4626)
#pragma  warning(disable:4668)
#pragma  warning(disable:4820)
#endif

#include <memory.h>
#include <time.h>

#if USE_POSIXSOCK
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#endif // USE_POSIXSOCK

#if USE_WINSOCK
#include <winsock2.h>
#include <Ws2tcpip.h>
#endif // USE_WINSOCK

#if USE_WINSSPI

#endif // USE_WINSSPI

#if USE_OPENSSL
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#endif // USE_OPENSSL

#ifdef _WIN32
#pragma  warning(pop)
#endif

#endif // SEABOLT_CONFIG_IMPL
