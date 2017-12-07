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


#include <logging.h>
#include "stdint.h"
#include "protocol_v1.h"


BoltProtocolV1Type BoltProtocolV1_markerType(uint8_t marker)
{
    if (marker < 0x80 || (marker >= 0xC8 && marker <= 0xCB) || marker >= 0xF0)
    {
        return BOLT_V1_INTEGER;
    }
    if ((marker >= 0x80 && marker <= 0x8F) || (marker >= 0xD0 && marker <= 0xD2))
    {
        return BOLT_V1_STRING;
    }
    if ((marker >= 0x90 && marker <= 0x9F) || (marker >= 0xD4 && marker <= 0xD6))
    {
        return BOLT_V1_LIST;
    }
    if ((marker >= 0xA0 && marker <= 0xAF) || (marker >= 0xD8 && marker <= 0xDA))
    {
        return BOLT_V1_MAP;
    }
    if ((marker >= 0xB0 && marker <= 0xBF) || (marker >= 0xDC && marker <= 0xDD))
    {
        return BOLT_V1_STRUCTURE;
    }
    switch(marker)
    {
        case 0xC0:
            return BOLT_V1_NULL;
        case 0xC1:
            return BOLT_V1_FLOAT;
        case 0xC2:
        case 0xC3:
            return BOLT_V1_BOOLEAN;
        case 0xCC:
        case 0xCD:
        case 0xCE:
            return BOLT_V1_BYTES;
        default:
            return BOLT_V1_RESERVED;
    }
}

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

int BoltProtocolV1_loadRun(BoltConnection* connection, const char* statement)
{
    BoltBuffer_load(connection->buffer, "\xB2\x10", 2);
    BoltProtocolV1_loadString(connection, statement, strlen(statement));
    BoltBuffer_load(connection->buffer, "\xA0", 2);
    BoltBuffer_pushStop(connection->buffer);
}

int BoltProtocolV1_loadPull(BoltConnection* connection)
{
    BoltBuffer_load(connection->buffer, "\xB0\x3F", 2);
    BoltBuffer_pushStop(connection->buffer);
}

int _unload(BoltConnection* connection, BoltValue* value);

int _unloadNull(BoltConnection* connection, BoltValue* value)
{
    uint8_t marker;
    BoltBuffer_unload_uint8(connection->buffer, &marker);
    if (marker == 0xC0)
    {
        BoltValue_toNull(value);
    }
    else
    {
        return -1;  // BOLT_ERROR_WRONG_TYPE
    }
}

int _unloadBoolean(BoltConnection* connection, BoltValue* value)
{
    uint8_t marker;
    BoltBuffer_unload_uint8(connection->buffer, &marker);
    if (marker == 0xC3)
    {
        BoltValue_toBit(value, 1);
    }
    else if (marker == 0xC2)
    {
        BoltValue_toBit(value, 0);
    }
    else
    {
        return -1;  // BOLT_ERROR_WRONG_TYPE
    }
}

int _unloadInteger(BoltConnection* connection, BoltValue* value)
{
    uint8_t marker;
    BoltBuffer_unload_uint8(connection->buffer, &marker);
    if (marker < 0x80)
    {
        BoltValue_toInt64(value, marker);
    }
    else if (marker >= 0xF0)
    {
        BoltValue_toInt64(value, marker - 0x100);
    }
    else if (marker == 0xC8)
    {
        int8_t x;
        BoltBuffer_unload_int8(connection->buffer, &x);
        BoltValue_toInt64(value, x);
    }
    else if (marker == 0xC9)
    {
        int16_t x;
        BoltBuffer_unload_int16be(connection->buffer, &x);
        BoltValue_toInt64(value, x);
    }
    else if (marker == 0xCA)
    {
        int32_t x;
        BoltBuffer_unload_int32be(connection->buffer, &x);
        BoltValue_toInt64(value, x);
    }
    else if (marker == 0xCB)
    {
        int64_t x;
        BoltBuffer_unload_int64be(connection->buffer, &x);
        BoltValue_toInt64(value, x);
    }
    else
    {
        return -1;  // BOLT_ERROR_WRONG_TYPE
    }
}

int _unloadString(BoltConnection* connection, BoltValue* value)
{
    uint8_t marker;
    BoltBuffer_unload_uint8(connection->buffer, &marker);
    if (marker >= 0x80 && marker <= 0x8F)
    {
        int32_t size;
        size = marker & 0x0F;
        BoltValue_toUTF8(value, NULL, size);
        BoltBuffer_unload(connection->buffer, BoltUTF8_get(value), size);
        return 0;
    }
    if (marker == 0xD0)
    {
        uint8_t size;
        BoltBuffer_unload_uint8(connection->buffer, &size);
        BoltValue_toUTF8(value, NULL, size);
        BoltBuffer_unload(connection->buffer, BoltUTF8_get(value), size);
        return 0;
    }
    if (marker == 0xD1)
    {
        uint16_t size;
        BoltBuffer_unload_uint16be(connection->buffer, &size);
        BoltValue_toUTF8(value, NULL, size);
        BoltBuffer_unload(connection->buffer, BoltUTF8_get(value), size);
        return 0;
    }
    if (marker == 0xD2)
    {
        int32_t size;
        BoltBuffer_unload_int32be(connection->buffer, &size);
        BoltValue_toUTF8(value, NULL, size);
        BoltBuffer_unload(connection->buffer, BoltUTF8_get(value), size);
        return 0;
    }
    log_error("Wrong marker type: %d", marker);
    return -1;  // BOLT_ERROR_WRONG_TYPE
}

int _unloadList(BoltConnection* connection, BoltValue* value)
{
    uint8_t marker;
    int32_t size;
    BoltBuffer_unload_uint8(connection->buffer, &marker);
    if (marker >= 0x90 && marker <= 0x9F)
    {
        size = marker & 0x0F;
        BoltValue_toList(value, size);
        for (int i = 0; i < size; i++)
        {
            _unload(connection, BoltList_value(value, i));
        }
        return 0;
    }
    // TODO: bigger lists
    return -1;  // BOLT_ERROR_WRONG_TYPE
}

int _unloadMap(BoltConnection* connection, BoltValue* value)
{
    uint8_t marker;
    int32_t size;
    BoltBuffer_unload_uint8(connection->buffer, &marker);
    if (marker >= 0xA0 && marker <= 0xAF)
    {
        size = marker & 0x0F;
        BoltValue_toUTF8Dictionary(value, size);
        for (int i = 0; i < size; i++)
        {
            _unload(connection, BoltUTF8Dictionary_key(value, i));
            _unload(connection, BoltUTF8Dictionary_value(value, i));
        }
        return 0;
    }
    // TODO: bigger maps
    return -1;  // BOLT_ERROR_WRONG_TYPE
}

int _unload(BoltConnection* connection, BoltValue* value)
{
    uint8_t marker;
    BoltBuffer_peek_uint8(connection->buffer, &marker);
    switch(BoltProtocolV1_markerType(marker))
    {
        case BOLT_V1_NULL:
            return _unloadNull(connection, value);
        case BOLT_V1_BOOLEAN:
            return _unloadBoolean(connection, value);
        case BOLT_V1_INTEGER:
            return _unloadInteger(connection, value);
        case BOLT_V1_STRING:
            return _unloadString(connection, value);
        case BOLT_V1_LIST:
            return _unloadList(connection, value);
        case BOLT_V1_MAP:
            return _unloadMap(connection, value);
        default:
            log_error("Unsupported marker: %d", marker);
            return -1;  // BOLT_UNSUPPORTED_MARKER
    }
}

int BoltProtocolV1_unload(BoltConnection* connection, BoltValue* value)
{
    uint8_t marker;
    uint8_t code;
    int32_t size;
    BoltBuffer_unload_uint8(connection->buffer, &marker);
    if (BoltProtocolV1_markerType(marker) == BOLT_V1_STRUCTURE)
    {
        size = marker & 0x0F;
        BoltBuffer_unload_uint8(connection->buffer, &code);
        if (code == 0x71)
        {
            if (size >= 1)
            {
                _unload(connection, value);
                if (size > 1)
                {
                    BoltValue* black_hole = BoltValue_create();
                    for (int i = 1; i < size; i++)
                    {
                        _unload(connection, black_hole);
                    }
                    BoltValue_destroy(black_hole);
                }
            }
            else
            {
                BoltValue_toNull(value);
            }
        }
        else
        {
            BoltValue_toSummary(value, code, size);
            for (int i = 0; i < size; i++)
            {
                _unload(connection, BoltSummary_value(value, i));
            }
        }
        return 0;

    }
    return -1;  // BOLT_ERROR_WRONG_TYPE
}
