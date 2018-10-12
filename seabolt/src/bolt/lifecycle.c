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

#include "bolt/config-impl.h"
#include "bolt/lifecycle.h"
#include "bolt/logging.h"

int SSL_CTX_TRUST_INDEX = -1;
int SSL_CTX_LOG_INDEX = -1;

void Bolt_startup()
{
#if USE_WINSOCK
    WSADATA data;
    WSAStartup(MAKEWORD(2, 2), &data);
#endif

#if USE_OPENSSL
    // initialize openssl
#if OPENSSL_VERSION_NUMBER<0x10100000L
    SSL_library_init();
#else
    OPENSSL_init_ssl(0, NULL);
#endif
    // load error strings and have cryto initialized
    SSL_load_error_strings();
    OpenSSL_add_all_algorithms();

    // register two ex_data indexes that we'll use to store
    // BoltTrust and BoltLog instances
    SSL_CTX_TRUST_INDEX = SSL_CTX_get_ex_new_index(0, NULL, NULL, NULL, NULL);
    SSL_CTX_LOG_INDEX = SSL_CTX_get_ex_new_index(0, NULL, NULL, NULL, NULL);
#endif
}

void Bolt_shutdown()
{
#if USE_WINSOCK
    WSACleanup();
#endif

#if USE_OPENSSL
    // we don't need any specific shutdown code for openssl
#endif
}