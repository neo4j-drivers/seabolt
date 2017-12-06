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


#include "stdint.h"
#include "protocol_v1.h"


int BoltProtocolV1_loadString(BoltConnection* connection, const char* string, int32_t size)
{
    // TODO: longer strings
    BoltBuffer_load(connection->buffer, "\xD0", 1);
    BoltBuffer_load_uint8(connection->buffer, (uint8_t)(size));
    BoltBuffer_load(connection->buffer, string, size);
}

int BoltProtocolV1_loadMap(BoltConnection* connection, BoltValue* value)
{
    // TODO: bigger maps
    BoltBuffer_load_uint8(connection->buffer, (uint8_t)(0xA0 + value->size));
    for (int32_t i = 0; i < value->size; i++)
    {
        BoltValue* key = BoltUTF8Dictionary_key(value, i);
        if (key != NULL)
        {
            const char* key_string = BoltUTF8_get(key);
            BoltProtocolV1_loadString(connection, key_string, key->size);
            BoltProtocolV1_load(connection, BoltUTF8Dictionary_value(value, i));
        }
    }
}

int BoltProtocolV1_load(BoltConnection* connection, BoltValue* value)
{
    switch (BoltValue_type(value))
    {
        case BOLT_UTF8:
            return BoltProtocolV1_loadString(connection, BoltUTF8_get(value), value->size);
        case BOLT_UTF8_DICTIONARY:
            return BoltProtocolV1_loadMap(connection, value);
        default:
            // TODO:  TYPE_NOT_SUPPORTED (vs TYPE_NOT_IMPLEMENTED)
            return EXIT_FAILURE;
    }
}

int BoltProtocolV1_loadInit(BoltConnection* connection, const char* user, const char* password)
{
    BoltBuffer_load(connection->buffer, "\xB2\x01", 2);
    BoltProtocolV1_loadString(connection, connection->user_agent, strlen(connection->user_agent));
    BoltValue* auth = BoltValue_create();
    BoltValue_toUTF8Dictionary(auth, 3);
    BoltValue_toUTF8(BoltUTF8Dictionary_withKey(auth, 0, "scheme", 6), "basic", 5);
    BoltValue_toUTF8(BoltUTF8Dictionary_withKey(auth, 1, "principal", 9), user, strlen(user));
    BoltValue_toUTF8(BoltUTF8Dictionary_withKey(auth, 2, "credentials", 11), password, strlen(password));
    BoltProtocolV1_load(connection, auth);
    BoltValue_destroy(auth);
    BoltBuffer_pushStop(connection->buffer);
}

int _unload(BoltConnection* connection, BoltValue* value)
{
    uint8_t marker;
    BoltBuffer_peek_uint8(connection->buffer, &marker);
    switch(marker)
    {
        case 0x80:
        case 0x81:
        case 0x82:
        case 0x83:
        case 0x84:
        case 0x85:
        case 0x86:
        case 0x87:
        case 0x88:
        case 0x89:
        case 0x8A:
        case 0x8B:
        case 0x8C:
        case 0x8D:
        case 0x8E:
        case 0x8F:
            return BoltProtocolV1_unloadString(connection, value);
        case 0xA0:
        case 0xA1:
        case 0xA2:
        case 0xA3:
        case 0xA4:
        case 0xA5:
        case 0xA6:
        case 0xA7:
        case 0xA8:
        case 0xA9:
        case 0xAA:
        case 0xAB:
        case 0xAC:
        case 0xAD:
        case 0xAE:
        case 0xAF:
            return BoltProtocolV1_unloadMap(connection, value);
        default:
            return -1;  // BOLT_ERROR_BAD_MARKER
    }
}

int BoltProtocolV1_unload(BoltConnection* connection, BoltValue* value)
{
    uint8_t marker;
    BoltBuffer_peek_uint8(connection->buffer, &marker);
    switch(marker)
    {
        case 0xB0:
        case 0xB1:
            // TODO: unload summary OR record
            return BoltProtocolV1_unloadSummary(connection, value);
        default:
            return -1;  // BOLT_ERROR_PROTOCOL_VIOLATION
    }
}

int BoltProtocolV1_unloadString(BoltConnection* connection, BoltValue* value)
{
    uint8_t marker;
    int32_t size;
    BoltBuffer_unload_uint8(connection->buffer, &marker);
    switch(marker)
    {
        case 0x80:
        case 0x81:
        case 0x82:
        case 0x83:
        case 0x84:
        case 0x85:
        case 0x86:
        case 0x87:
        case 0x88:
        case 0x89:
        case 0x8A:
        case 0x8B:
        case 0x8C:
        case 0x8D:
        case 0x8E:
        case 0x8F:
            size = marker & 0x0F;
            BoltValue_toUTF8(value, NULL, size);
            BoltBuffer_unload(connection->buffer, BoltUTF8_get(value), size);
            return 0;
        default:
            return -1;  // BOLT_ERROR_WRONG_TYPE
    }
}

int BoltProtocolV1_unloadMap(BoltConnection* connection, BoltValue* value)
{
    uint8_t marker;
    int32_t size;
    BoltBuffer_unload_uint8(connection->buffer, &marker);
    switch(marker)
    {
        case 0xA0:
        case 0xA1:
        case 0xA2:
        case 0xA3:
        case 0xA4:
        case 0xA5:
        case 0xA6:
        case 0xA7:
        case 0xA8:
        case 0xA9:
        case 0xAA:
        case 0xAB:
        case 0xAC:
        case 0xAD:
        case 0xAE:
        case 0xAF:
            size = marker & 0x0F;
            BoltValue_toUTF8Dictionary(value, size);
            for (int i = 0; i < size; i++)
            {
                _unload(connection, BoltUTF8Dictionary_key(value, i));
                _unload(connection, BoltUTF8Dictionary_value(value, i));
            }
            return 0;
        default:
            return -1;  // BOLT_ERROR_WRONG_TYPE
    }
}

int BoltProtocolV1_unloadSummary(BoltConnection* connection, BoltValue* value)
{
    uint8_t marker;
    uint8_t code;
    int32_t size;
    BoltBuffer_unload_uint8(connection->buffer, &marker);
    switch(marker)
    {
        case 0xB0:
        case 0xB1:
            size = marker & 0x0F;
            BoltBuffer_unload_uint8(connection->buffer, &code);
            BoltValue_toSummary(value, code, size);
            for (int i = 0; i < size; i++)
            {
                _unload(connection, BoltSummary_value(value, i));
            }
            return 0;
        default:
            return -1;  // BOLT_ERROR_WRONG_TYPE
    }
}
