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


#include <string.h>

#include "auth.h"

BoltValue* BoltAuth_basic(const char* username, const char* password, const char* realm)
{
    struct BoltValue* auth_token = BoltValue_create();
    BoltValue_format_as_Dictionary(auth_token, realm==NULL ? 3 : 4);
    BoltDictionary_set_key(auth_token, 0, "scheme", 6);
    BoltValue_format_as_String(BoltDictionary_value(auth_token, 0), "basic", 5);
    BoltDictionary_set_key(auth_token, 1, "principal", 9);
    BoltValue_format_as_String(BoltDictionary_value(auth_token, 1), username, (int32_t) strlen(username));
    BoltDictionary_set_key(auth_token, 2, "credentials", 11);
    BoltValue_format_as_String(BoltDictionary_value(auth_token, 2), password, (int32_t) strlen(password));
    if (realm!=NULL) {
        BoltDictionary_set_key(auth_token, 3, "realm", 5);
        BoltValue_format_as_String(BoltDictionary_value(auth_token, 3), realm, (int32_t) strlen(realm));
    }
    return auth_token;
}

BoltValue* BoltAuth_none()
{
    struct BoltValue* auth_token = BoltValue_create();
    BoltValue_format_as_Dictionary(auth_token, 1);
    BoltDictionary_set_key(auth_token, 0, "scheme", 6);
    BoltValue_format_as_String(BoltDictionary_value(auth_token, 0), "none", 4);
    return auth_token;
}
