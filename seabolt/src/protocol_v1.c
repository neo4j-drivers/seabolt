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


#include <stdlib.h>
#include <bolt.h>
#include <memory.h>
#include "buffer.h"
#include "protocol_v1.h"


enum BoltProtocolV1Type BoltProtocolV1_marker_type(uint8_t marker)
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

int BoltProtocolV1_load_null(struct BoltConnection* connection)
{
    BoltBuffer_load_uint8(connection->tx_buffer_1, 0xC0);
}

int BoltProtocolV1_load_boolean(struct BoltConnection* connection, int value)
{
    BoltBuffer_load_uint8(connection->tx_buffer_1, (value == 0) ? (uint8_t)(0xC2) : (uint8_t)(0xC3));
}

int BoltProtocolV1_load_integer(struct BoltConnection* connection, int64_t value)
{
    if (value >= -0x10 && value < 0x80)
    {
        BoltBuffer_load_int8(connection->tx_buffer_1, value);
    }
    else if (value >= -0x80 && value < -0x10)
    {
        BoltBuffer_load_uint8(connection->tx_buffer_1, 0xC8);
        BoltBuffer_load_int8(connection->tx_buffer_1, value);
    }
    else if (value >= -0x8000 && value < 0x8000)
    {
        BoltBuffer_load_uint8(connection->tx_buffer_1, 0xC9);
        BoltBuffer_load_int16_be(connection->tx_buffer_1, value);
    }
    else if (value >= -0x80000000L && value < 0x80000000L)
    {
        BoltBuffer_load_uint8(connection->tx_buffer_1, 0xCA);
        BoltBuffer_load_int32_be(connection->tx_buffer_1, value);
    }
    else
    {
        BoltBuffer_load_uint8(connection->tx_buffer_1, 0xCB);
        BoltBuffer_load_int64_be(connection->tx_buffer_1, value);
    }
}

int BoltProtocolV1_load_float(struct BoltConnection* connection, double value)
{
    BoltBuffer_load_uint8(connection->tx_buffer_1, 0xC1);
    BoltBuffer_load_double_be(connection->tx_buffer_1, value);
}

int BoltProtocolV1_load_bytes(struct BoltConnection* connection, const char* string, int32_t size)
{
    if (size < 0)
    {
        return -1;
    }
    if (size < 0x100)
    {
        BoltBuffer_load_uint8(connection->tx_buffer_1, 0xCC);
        BoltBuffer_load_uint8(connection->tx_buffer_1, (uint8_t)(size));
        BoltBuffer_load(connection->tx_buffer_1, string, size);
    }
    else if (size < 0x10000)
    {
        BoltBuffer_load_uint8(connection->tx_buffer_1, 0xCD);
        BoltBuffer_load_uint16_be(connection->tx_buffer_1, (uint16_t)(size));
        BoltBuffer_load(connection->tx_buffer_1, string, size);
    }
    else
    {
        BoltBuffer_load_uint8(connection->tx_buffer_1, 0xCE);
        BoltBuffer_load_int32_be(connection->tx_buffer_1, size);
        BoltBuffer_load(connection->tx_buffer_1, string, size);
    }
}

int BoltProtocolV1_load_string(struct BoltConnection* connection, const char* string, int32_t size)
{
    if (size < 0)
    {
        return -1;
    }
    if (size < 0x10)
    {
        BoltBuffer_load_uint8(connection->tx_buffer_1, (uint8_t)(0x80 + size));
        BoltBuffer_load(connection->tx_buffer_1, string, size);
    }
    else if (size < 0x100)
    {
        BoltBuffer_load_uint8(connection->tx_buffer_1, 0xD0);
        BoltBuffer_load_uint8(connection->tx_buffer_1, (uint8_t)(size));
        BoltBuffer_load(connection->tx_buffer_1, string, size);
    }
    else if (size < 0x10000)
    {
        BoltBuffer_load_uint8(connection->tx_buffer_1, 0xD1);
        BoltBuffer_load_uint16_be(connection->tx_buffer_1, (uint16_t)(size));
        BoltBuffer_load(connection->tx_buffer_1, string, size);
    }
    else
    {
        BoltBuffer_load_uint8(connection->tx_buffer_1, 0xD2);
        BoltBuffer_load_int32_be(connection->tx_buffer_1, size);
        BoltBuffer_load(connection->tx_buffer_1, string, size);
    }
}

int _load_list_header(struct BoltConnection* connection, int32_t size)
{
    if (size < 0)
    {
        return -1;
    }
    if (size < 0x10)
    {
        BoltBuffer_load_uint8(connection->tx_buffer_1, (uint8_t)(0x90 + size));
    }
    else if (size < 0x100)
    {
        BoltBuffer_load_uint8(connection->tx_buffer_1, 0xD4);
        BoltBuffer_load_uint8(connection->tx_buffer_1, (uint8_t)(size));
    }
    else if (size < 0x10000)
    {
        BoltBuffer_load_uint8(connection->tx_buffer_1, 0xD5);
        BoltBuffer_load_uint16_be(connection->tx_buffer_1, (uint16_t)(size));
    }
    else
    {
        BoltBuffer_load_uint8(connection->tx_buffer_1, 0xD6);
        BoltBuffer_load_int32_be(connection->tx_buffer_1, size);
    }
}

int _load_map_header(struct BoltConnection* connection, int32_t size)
{
    if (size < 0)
    {
        return -1;
    }
    if (size < 0x10)
    {
        BoltBuffer_load_uint8(connection->tx_buffer_1, (uint8_t)(0xA0 + size));
    }
    else if (size < 0x100)
    {
        BoltBuffer_load_uint8(connection->tx_buffer_1, 0xD8);
        BoltBuffer_load_uint8(connection->tx_buffer_1, (uint8_t)(size));
    }
    else if (size < 0x10000)
    {
        BoltBuffer_load_uint8(connection->tx_buffer_1, 0xD9);
        BoltBuffer_load_uint16_be(connection->tx_buffer_1, (uint16_t)(size));
    }
    else
    {
        BoltBuffer_load_uint8(connection->tx_buffer_1, 0xDA);
        BoltBuffer_load_int32_be(connection->tx_buffer_1, size);
    }
}

int _load_structure_header(struct BoltConnection* connection, int16_t code, int8_t size)
{
    if (code < 0 || size < 0 || size >= 0x10)
    {
        return -1;
    }
    BoltBuffer_load_uint8(connection->tx_buffer_1, (uint8_t)(0xB0 + size));
    BoltBuffer_load_int8(connection->tx_buffer_1, code);
}

int _add_chunks_to_pending(struct BoltConnection* connection)
{
    // TODO: more chunks if size is too big
    int size = BoltBuffer_unloadable(connection->tx_buffer_1);
    char header[2];
    header[0] = (char)(size >> 8);
    header[1] = (char)(size);
    BoltBuffer_load(connection->tx_buffer_0, &header[0], sizeof(header));
    BoltBuffer_load(connection->tx_buffer_0, BoltBuffer_unload_target(connection->tx_buffer_1, size), size);
    header[0] = (char)(0);
    header[1] = (char)(0);
    BoltBuffer_load(connection->tx_buffer_0, &header[0], sizeof(header));
    BoltBuffer_compact(connection->tx_buffer_1);
}

int BoltProtocolV1_load(struct BoltConnection* connection, struct BoltValue* value)
{
    switch (BoltValue_type(value))
    {
        case BOLT_NULL:
            return BoltProtocolV1_load_null(connection);
        case BOLT_BIT:
            return BoltProtocolV1_load_boolean(connection, BoltBit_get(value));
        case BOLT_BIT_ARRAY:
        {
            try(_load_list_header(connection, value->size));
            for (int32_t i = 0; i < value->size; i++)
            {
                try(BoltProtocolV1_load_boolean(connection, BoltBitArray_get(value, i)));
            }
            return 0;
        }
        case BOLT_BYTE:
            // A Byte is coerced to an Integer (Int64)
            return BoltProtocolV1_load_integer(connection, BoltByte_get(value));
        case BOLT_BYTE_ARRAY:
            return BoltProtocolV1_load_bytes(connection, BoltByteArray_get_all(value), value->size);
        case BOLT_CHAR16:
            return -1;
        case BOLT_CHAR32:
            return -1;
        case BOLT_CHAR16_ARRAY:
            return -1;
        case BOLT_CHAR32_ARRAY:
            return -1;
        case BOLT_UTF8:
            return BoltProtocolV1_load_string(connection, BoltUTF8_get(value), value->size);
        case BOLT_UTF16:
            return -1;
        case BOLT_UTF8_ARRAY:
            return -1;
        case BOLT_UTF16_ARRAY:
            return -1;
        case BOLT_UTF8_DICTIONARY:
        {
            try(_load_map_header(connection, value->size));
            for (int32_t i = 0; i < value->size; i++)
            {
                struct BoltValue* key = BoltUTF8Dictionary_key(value, i);
                if (key != NULL)
                {
                    const char* key_string = BoltUTF8_get(key);
                    try(BoltProtocolV1_load_string(connection, key_string, key->size));
                    try(BoltProtocolV1_load(connection, BoltUTF8Dictionary_value(value, i)));
                }
            }
            return 0;
        }
        case BOLT_UTF16_DICTIONARY:
            return -1;
        case BOLT_NUM8:
            return BoltProtocolV1_load_integer(connection, BoltNum8_get(value));
        case BOLT_NUM16:
            return BoltProtocolV1_load_integer(connection, BoltNum16_get(value));
        case BOLT_NUM32:
            return BoltProtocolV1_load_integer(connection, BoltNum32_get(value));
        case BOLT_NUM64:
            return -1;  // An int64 can't (necessarily) hold a num64; coerce to string?
        case BOLT_NUM8_ARRAY:
            return -1;
        case BOLT_NUM16_ARRAY:
            return -1;
        case BOLT_NUM32_ARRAY:
            return -1;
        case BOLT_NUM64_ARRAY:
            return -1;
        case BOLT_INT8:
            return BoltProtocolV1_load_integer(connection, BoltInt8_get(value));
        case BOLT_INT16:
            return BoltProtocolV1_load_integer(connection, BoltInt16_get(value));
        case BOLT_INT32:
            return BoltProtocolV1_load_integer(connection, BoltInt32_get(value));
        case BOLT_INT64:
            return BoltProtocolV1_load_integer(connection, BoltInt64_get(value));
        case BOLT_INT8_ARRAY:
            return -1;
        case BOLT_INT16_ARRAY:
            return -1;
        case BOLT_INT32_ARRAY:
            return -1;
        case BOLT_INT64_ARRAY:
            return -1;
        case BOLT_FLOAT32:
            return BoltProtocolV1_load_float(connection, BoltFloat32_get(value));
        case BOLT_FLOAT32_PAIR:
            return -1;
        case BOLT_FLOAT32_TRIPLE:
            return -1;
        case BOLT_FLOAT32_QUAD:
            return -1;
        case BOLT_FLOAT32_ARRAY:
            return -1;
        case BOLT_FLOAT32_PAIR_ARRAY:
            return -1;
        case BOLT_FLOAT32_TRIPLE_ARRAY:
            return -1;
        case BOLT_FLOAT32_QUAD_ARRAY:
            return -1;
        case BOLT_FLOAT64:
            return BoltProtocolV1_load_float(connection, BoltFloat64_get(value));
        case BOLT_FLOAT64_PAIR:
//        {
//            try(_load_list_header(connection, 2));
//            try(BoltProtocolV1_load_float(connection, BoltFloat64Pair_get_first(value)));
//            try(BoltProtocolV1_load_float(connection, BoltFloat64Pair_get_second(value)));
//            return 0;
//        }
            return -1;
        case BOLT_FLOAT64_TRIPLE:
//        {
//            try(_load_list_header(connection, 3));
//            try(BoltProtocolV1_load_float(connection, BoltFloat64Triple_get_first(value)));
//            try(BoltProtocolV1_load_float(connection, BoltFloat64Triple_get_second(value)));
//            try(BoltProtocolV1_load_float(connection, BoltFloat64Triple_get_third(value)));
//            return 0;
//        }
            return -1;
        case BOLT_FLOAT64_QUAD:
            return -1;
        case BOLT_FLOAT64_ARRAY:
            return -1;
        case BOLT_FLOAT64_PAIR_ARRAY:
            return -1;
        case BOLT_FLOAT64_TRIPLE_ARRAY:
            return -1;
        case BOLT_FLOAT64_QUAD_ARRAY:
            return -1;
        case BOLT_STRUCTURE:
        {
            try(_load_structure_header(connection, BoltStructure_code(value), value->size));
            for (int32_t i = 0; i < value->size; i++)
            {
                try(BoltProtocolV1_load(connection, BoltStructure_value(value, i)));
            }
            return 0;
        }
        case BOLT_STRUCTURE_ARRAY:
            return -1;
        case BOLT_REQUEST:
        {
            try(_load_structure_header(connection, BoltRequest_code(value), value->size));
            for (int32_t i = 0; i < value->size; i++)
            {
                try(BoltProtocolV1_load(connection, BoltRequest_value(value, i)));
            }
            _add_chunks_to_pending(connection);
            return 0;
        }
        case BOLT_SUMMARY:
            return -1;
        case BOLT_LIST:
        {
            try(_load_list_header(connection, value->size));
            for (int32_t i = 0; i < value->size; i++)
            {
                try(BoltProtocolV1_load(connection, BoltList_value(value, i)));
            }
            return 0;
        }
        default:
            // TODO:  TYPE_NOT_SUPPORTED (vs TYPE_NOT_IMPLEMENTED)
            return EXIT_FAILURE;
    }
}

int BoltProtocolV1_compile_INIT(struct BoltValue* value, const char* user_agent, const char* user, const char* password)
{
    BoltValue_to_Request(value, 0x01, 2);
    BoltValue_to_UTF8(BoltRequest_value(value, 0), user_agent, strlen(user_agent));
    struct BoltValue* auth = BoltRequest_value(value, 1);
    if (user == NULL || password == NULL)
    {
        BoltValue_to_UTF8Dictionary(auth, 0);
    }
    else
    {
        BoltValue_to_UTF8Dictionary(auth, 3);
        BoltValue_to_UTF8(BoltUTF8Dictionary_with_key(auth, 0, "scheme", 6), "basic", 5);
        BoltValue_to_UTF8(BoltUTF8Dictionary_with_key(auth, 1, "principal", 9), user, strlen(user));
        BoltValue_to_UTF8(BoltUTF8Dictionary_with_key(auth, 2, "credentials", 11), password, strlen(password));
    }
}

int BoltProtocolV1_compile_RUN(struct BoltValue* value, const char* statement, struct BoltValue** parameters)
{
    BoltValue_to_Request(value, 0x10, 2);
    BoltValue_to_UTF8(BoltRequest_value(value, 0), statement, strlen(statement));
    *parameters = BoltRequest_value(value, 1);
    BoltValue_to_UTF8Dictionary(*parameters, 0);
}

int BoltProtocolV1_compile_DISCARD_ALL(struct BoltValue* value)
{
    BoltValue_to_Request(value, 0x2F, 0);
}

int BoltProtocolV1_compile_PULL_ALL(struct BoltValue* value)
{
    BoltValue_to_Request(value, 0x3F, 0);
}

int _unload(struct BoltConnection* connection, struct BoltValue* value);

int _unload_null(struct BoltConnection* connection, struct BoltValue* value)
{
    uint8_t marker;
    BoltBuffer_unload_uint8(connection->rx_buffer_1, &marker);
    if (marker == 0xC0)
    {
        BoltValue_to_Null(value);
    }
    else
    {
        return -1;  // BOLT_ERROR_WRONG_TYPE
    }
}

int _unload_boolean(struct BoltConnection* connection, struct BoltValue* value)
{
    uint8_t marker;
    BoltBuffer_unload_uint8(connection->rx_buffer_1, &marker);
    if (marker == 0xC3)
    {
        BoltValue_to_Bit(value, 1);
    }
    else if (marker == 0xC2)
    {
        BoltValue_to_Bit(value, 0);
    }
    else
    {
        return -1;  // BOLT_ERROR_WRONG_TYPE
    }
}

int _unload_integer(struct BoltConnection* connection, struct BoltValue* value)
{
    uint8_t marker;
    BoltBuffer_unload_uint8(connection->rx_buffer_1, &marker);
    if (marker < 0x80)
    {
        BoltValue_to_Int64(value, marker);
    }
    else if (marker >= 0xF0)
    {
        BoltValue_to_Int64(value, marker - 0x100);
    }
    else if (marker == 0xC8)
    {
        int8_t x;
        BoltBuffer_unload_int8(connection->rx_buffer_1, &x);
        BoltValue_to_Int64(value, x);
    }
    else if (marker == 0xC9)
    {
        int16_t x;
        BoltBuffer_unload_int16_be(connection->rx_buffer_1, &x);
        BoltValue_to_Int64(value, x);
    }
    else if (marker == 0xCA)
    {
        int32_t x;
        BoltBuffer_unload_int32_be(connection->rx_buffer_1, &x);
        BoltValue_to_Int64(value, x);
    }
    else if (marker == 0xCB)
    {
        int64_t x;
        BoltBuffer_unload_int64_be(connection->rx_buffer_1, &x);
        BoltValue_to_Int64(value, x);
    }
    else
    {
        return -1;  // BOLT_ERROR_WRONG_TYPE
    }
}

int _unload_float(struct BoltConnection* connection, struct BoltValue* value)
{
    uint8_t marker;
    BoltBuffer_unload_uint8(connection->rx_buffer_1, &marker);
    if (marker == 0xC1)
    {
        double x;
        BoltBuffer_unload_double_be(connection->rx_buffer_1, &x);
        BoltValue_to_Float64(value, x);
    }
    else
    {
        return -1;  // BOLT_ERROR_WRONG_TYPE
    }
}

int _unload_string(struct BoltConnection* connection, struct BoltValue* value)
{
    uint8_t marker;
    BoltBuffer_unload_uint8(connection->rx_buffer_1, &marker);
    if (marker >= 0x80 && marker <= 0x8F)
    {
        int32_t size;
        size = marker & 0x0F;
        BoltValue_to_UTF8(value, NULL, size);
        BoltBuffer_unload(connection->rx_buffer_1, BoltUTF8_get(value), size);
        return 0;
    }
    if (marker == 0xD0)
    {
        uint8_t size;
        BoltBuffer_unload_uint8(connection->rx_buffer_1, &size);
        BoltValue_to_UTF8(value, NULL, size);
        BoltBuffer_unload(connection->rx_buffer_1, BoltUTF8_get(value), size);
        return 0;
    }
    if (marker == 0xD1)
    {
        uint16_t size;
        BoltBuffer_unload_uint16_be(connection->rx_buffer_1, &size);
        BoltValue_to_UTF8(value, NULL, size);
        BoltBuffer_unload(connection->rx_buffer_1, BoltUTF8_get(value), size);
        return 0;
    }
    if (marker == 0xD2)
    {
        int32_t size;
        BoltBuffer_unload_int32_be(connection->rx_buffer_1, &size);
        BoltValue_to_UTF8(value, NULL, size);
        BoltBuffer_unload(connection->rx_buffer_1, BoltUTF8_get(value), size);
        return 0;
    }
    BoltLog_error("[NET] Wrong marker type: %d", marker);
    return -1;  // BOLT_ERROR_WRONG_TYPE
}

int _unload_list(struct BoltConnection* connection, struct BoltValue* value)
{
    uint8_t marker;
    int32_t size;
    BoltBuffer_unload_uint8(connection->rx_buffer_1, &marker);
    if (marker >= 0x90 && marker <= 0x9F)
    {
        size = marker & 0x0F;
        BoltValue_to_List(value, size);
        for (int i = 0; i < size; i++)
        {
            _unload(connection, BoltList_value(value, i));
        }
        return 0;
    }
    // TODO: bigger lists
    return -1;  // BOLT_ERROR_WRONG_TYPE
}

int _unload_map(struct BoltConnection* connection, struct BoltValue* value)
{
    uint8_t marker;
    int32_t size;
    BoltBuffer_unload_uint8(connection->rx_buffer_1, &marker);
    if (marker >= 0xA0 && marker <= 0xAF)
    {
        size = marker & 0x0F;
        BoltValue_to_UTF8Dictionary(value, size);
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

int _unload_structure(struct BoltConnection* connection, struct BoltValue* value)
{
    uint8_t marker;
    int8_t code;
    int32_t size;
    BoltBuffer_unload_uint8(connection->rx_buffer_1, &marker);
    if (marker >= 0xB0 && marker <= 0xBF)
    {
        size = marker & 0x0F;
        BoltBuffer_unload_int8(connection->rx_buffer_1, &code);
        BoltValue_to_Structure(value, code, size);
        for (int i = 0; i < size; i++)
        {
            _unload(connection, BoltStructure_value(value, i));
        }
        return 0;
    }
    // TODO: bigger structures (that are never actually used)
    return -1;  // BOLT_ERROR_WRONG_TYPE
}

int _unload(struct BoltConnection* connection, struct BoltValue* value)
{
    uint8_t marker;
    BoltBuffer_peek_uint8(connection->rx_buffer_1, &marker);
    switch(BoltProtocolV1_marker_type(marker))
    {
        case BOLT_V1_NULL:
            return _unload_null(connection, value);
        case BOLT_V1_BOOLEAN:
            return _unload_boolean(connection, value);
        case BOLT_V1_INTEGER:
            return _unload_integer(connection, value);
        case BOLT_V1_FLOAT:
            return _unload_float(connection, value);
        case BOLT_V1_STRING:
            return _unload_string(connection, value);
        case BOLT_V1_LIST:
            return _unload_list(connection, value);
        case BOLT_V1_MAP:
            return _unload_map(connection, value);
        case BOLT_V1_STRUCTURE:
            return _unload_structure(connection, value);
        default:
            BoltLog_error("[NET] Unsupported marker: %d", marker);
            return -1;  // BOLT_UNSUPPORTED_MARKER
    }
}

int BoltProtocolV1_unload(struct BoltConnection* connection, struct BoltValue* value)
{
    if (BoltBuffer_unloadable(connection->rx_buffer_1) == 0)
    {
        return 0;
    }
    uint8_t marker;
    uint8_t code;
    int32_t size;
    BoltBuffer_unload_uint8(connection->rx_buffer_1, &marker);
    if (BoltProtocolV1_marker_type(marker) == BOLT_V1_STRUCTURE)
    {
        size = marker & 0x0F;
        BoltBuffer_unload_uint8(connection->rx_buffer_1, &code);
        if (code == 0x71)  // RECORD
        {
            if (size >= 1)
            {
                _unload(connection, value);
                if (size > 1)
                {
                    struct BoltValue* black_hole = BoltValue_create();
                    for (int i = 1; i < size; i++)
                    {
                        _unload(connection, black_hole);
                    }
                    BoltValue_destroy(black_hole);
                }
            }
            else
            {
                BoltValue_to_Null(value);
            }
        }
        else
        {
            BoltValue_to_Summary(value, code, size);
            for (int i = 0; i < size; i++)
            {
                _unload(connection, BoltSummary_value(value, i));
            }
        }
        return 1;

    }
    return -1;  // BOLT_ERROR_WRONG_TYPE
}

const char* BoltProtocolV1_structure_name(int16_t code)
{
    switch(code)
    {
        case 'N':
            return "Node";
        case 'R':
            return "Relationship";
        case 'r':
            return "UnboundRelationship";
        case 'P':
            return "Path";
        default:
            return NULL;
    }
}

const char* BoltProtocolV1_request_name(int16_t code)
{
    switch(code)
    {
        case 0x01:
            return "INIT";
        case 0x0E:
            return "ACK_FAILURE";
        case 0x0F:
            return "RESET";
        case 0x10:
            return "RUN";
        case 0x2F:
            return "DISCARD_ALL";
        case 0x3F:
            return "PULL_ALL";
        default:
            return NULL;
    }
}

const char* BoltProtocolV1_summary_name(int16_t code)
{
    switch(code)
    {
        case 0x70:
            return "SUCCESS";
        case 0x7E:
            return "IGNORED";
        case 0x7F:
            return "FAILURE";
        default:
            return NULL;
    }
}
