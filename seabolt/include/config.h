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
#ifndef SEABOLT_CONFIG
#define SEABOLT_CONFIG
#include "config-options.h"

#define BOLT_PUBLIC_API

#if USE_POSIXSOCK
#include <netdb.h>
#endif

#ifdef WIN32
typedef unsigned short in_port_t;
#define BOLT_PUBLIC_API __declspec(dllexport)
#endif // USE_WINSOCK

#endif // SEABOLT_CONFIG
