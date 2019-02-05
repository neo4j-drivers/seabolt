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


#ifndef SEABOLT_AUTH
#define SEABOLT_AUTH

#include "values.h"

/**
 * Generates an authentication token that can be used for basic authentication use-cases that consists of
 * username, password and an optional realm.
 *
 * Returned \ref BoltValue is a dictionary that contains the following key-value pairs.
 * @verbatim embed:rst:leading-asterisk
 * =============  ==============================
 * Key            Value
 * =============  ==============================
 * "scheme"       "basic"
 * "principal"    username
 * "credentials"  password
 * "realm"        realm (only if it's not NULL)
 * =============  ==============================
 * @endverbatim
 *
 * @param username username
 * @param password password
 * @param realm realm if available (may be passed as NULL)
 * @return a constructed \ref BoltValue that can be used as an authentication token
 */
SEABOLT_EXPORT BoltValue* BoltAuth_basic(const char* username, const char* password, const char* realm);

/**
 * Generates an authentication token that can be used towards servers that has disabled authentication
 *
 * Returned \ref BoltValue is a dictionary that contains the following key-value pairs.
 * @verbatim embed:rst:leading-asterisk
 * =============  ==============================
 * Key            Value
 * =============  ==============================
 * "scheme"       "none"
 * =============  ==============================
 * @endverbatim
 *
 * @return a constructed \ref BoltValue that can be used as an authentication token
 */
SEABOLT_EXPORT BoltValue* BoltAuth_none();

#endif // SEABOLT_AUTH
