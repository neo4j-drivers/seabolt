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
#include <values.h>
#include <memory.h>
#include <assert.h>
#include "../buffer.h"
#include "v1.h"
#include "mem.h"
#include "logging.h"

#define RUN 0x10
#define DISCARD_ALL 0x2F
#define PULL_ALL 0x3F

#define INITIAL_TX_BUFFER_SIZE 8192
#define INITIAL_RX_BUFFER_SIZE 8192


void _create_run_request(struct _run_request* run, int32_t n_parameters)
{
    run->request = BoltValue_create();
    BoltValue_to_Request(run->request, RUN, 2);
    run->statement = BoltRequest_value(run->request, 0);
    run->parameters = BoltRequest_value(run->request, 1);
    BoltValue_to_Dictionary8(run->parameters, n_parameters);
}

struct BoltProtocolV1State* BoltProtocolV1_create_state()
{
    struct BoltProtocolV1State* state = BoltMem_allocate(sizeof(struct BoltProtocolV1State));

    state->tx_buffer = BoltBuffer_create(INITIAL_TX_BUFFER_SIZE);
    state->rx_buffer = BoltBuffer_create(INITIAL_RX_BUFFER_SIZE);

    state->next_request_id = 0;
    state->response_counter = 0;

    _create_run_request(&state->run, 0);
    _create_run_request(&state->begin, 0);
    BoltValue_to_String8(state->begin.statement, "BEGIN", 5);
    _create_run_request(&state->commit, 0);
    BoltValue_to_String8(state->commit.statement, "COMMIT", 6);
    _create_run_request(&state->rollback, 0);
    BoltValue_to_String8(state->rollback.statement, "ROLLBACK", 8);

    state->discard_request = BoltValue_create();
    BoltValue_to_Request(state->discard_request, DISCARD_ALL, 0);

    state->pull_request = BoltValue_create();
    BoltValue_to_Request(state->pull_request, PULL_ALL, 0);

    state->fetched = BoltValue_create();
    return state;
}

void BoltProtocolV1_destroy_state(struct BoltProtocolV1State* state)
{
    if (state == NULL) return;

    BoltBuffer_destroy(state->tx_buffer);
    BoltBuffer_destroy(state->rx_buffer);

    BoltValue_destroy(state->run.request);
    BoltValue_destroy(state->begin.request);
    BoltValue_destroy(state->commit.request);
    BoltValue_destroy(state->rollback.request);

    BoltValue_destroy(state->discard_request);
    BoltValue_destroy(state->pull_request);

    BoltValue_destroy(state->fetched);

    BoltMem_deallocate(state, sizeof(struct BoltProtocolV1State));
}

struct BoltProtocolV1State* BoltProtocolV1_state(struct BoltConnection* connection)
{
    return (struct BoltProtocolV1State*)(connection->protocol_state);
}

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
    struct BoltProtocolV1State* state = BoltProtocolV1_state(connection);
    BoltBuffer_load_uint8(state->tx_buffer, 0xC0);
    return 0;
}

int BoltProtocolV1_load_boolean(struct BoltConnection* connection, int value)
{
    struct BoltProtocolV1State* state = BoltProtocolV1_state(connection);
    BoltBuffer_load_uint8(state->tx_buffer, (value == 0) ? (uint8_t)(0xC2) : (uint8_t)(0xC3));
    return 0;
}

int BoltProtocolV1_load_integer(struct BoltConnection* connection, int64_t value)
{
    struct BoltProtocolV1State* state = BoltProtocolV1_state(connection);
    if (value >= -0x10 && value < 0x80)
    {
        BoltBuffer_load_int8(state->tx_buffer, value);
    }
    else if (value >= -0x80 && value < -0x10)
    {
        BoltBuffer_load_uint8(state->tx_buffer, 0xC8);
        BoltBuffer_load_int8(state->tx_buffer, value);
    }
    else if (value >= -0x8000 && value < 0x8000)
    {
        BoltBuffer_load_uint8(state->tx_buffer, 0xC9);
        BoltBuffer_load_int16_be(state->tx_buffer, value);
    }
    else if (value >= -0x80000000L && value < 0x80000000L)
    {
        BoltBuffer_load_uint8(state->tx_buffer, 0xCA);
        BoltBuffer_load_int32_be(state->tx_buffer, value);
    }
    else
    {
        BoltBuffer_load_uint8(state->tx_buffer, 0xCB);
        BoltBuffer_load_int64_be(state->tx_buffer, value);
    }
    return 0;
}

int BoltProtocolV1_load_float(struct BoltConnection* connection, double value)
{
    struct BoltProtocolV1State* state = BoltProtocolV1_state(connection);
    BoltBuffer_load_uint8(state->tx_buffer, 0xC1);
    BoltBuffer_load_double_be(state->tx_buffer, value);
    return 0;
}

int BoltProtocolV1_load_bytes(struct BoltConnection* connection, const char* string, int32_t size)
{
    if (size < 0)
    {
        return -1;
    }
    struct BoltProtocolV1State* state = BoltProtocolV1_state(connection);
    if (size < 0x100)
    {
        BoltBuffer_load_uint8(state->tx_buffer, 0xCC);
        BoltBuffer_load_uint8(state->tx_buffer, (uint8_t)(size));
        BoltBuffer_load(state->tx_buffer, string, size);
    }
    else if (size < 0x10000)
    {
        BoltBuffer_load_uint8(state->tx_buffer, 0xCD);
        BoltBuffer_load_uint16_be(state->tx_buffer, (uint16_t)(size));
        BoltBuffer_load(state->tx_buffer, string, size);
    }
    else
    {
        BoltBuffer_load_uint8(state->tx_buffer, 0xCE);
        BoltBuffer_load_int32_be(state->tx_buffer, size);
        BoltBuffer_load(state->tx_buffer, string, size);
    }
    return 0;
}

int BoltProtocolV1_load_string(struct BoltConnection* connection, const char* string, int32_t size)
{
    if (size < 0)
    {
        return -1;
    }
    struct BoltProtocolV1State* state = BoltProtocolV1_state(connection);
    if (size < 0x10)
    {
        BoltBuffer_load_uint8(state->tx_buffer, (uint8_t)(0x80 + size));
        BoltBuffer_load(state->tx_buffer, string, size);
    }
    else if (size < 0x100)
    {
        BoltBuffer_load_uint8(state->tx_buffer, 0xD0);
        BoltBuffer_load_uint8(state->tx_buffer, (uint8_t)(size));
        BoltBuffer_load(state->tx_buffer, string, size);
    }
    else if (size < 0x10000)
    {
        BoltBuffer_load_uint8(state->tx_buffer, 0xD1);
        BoltBuffer_load_uint16_be(state->tx_buffer, (uint16_t)(size));
        BoltBuffer_load(state->tx_buffer, string, size);
    }
    else
    {
        BoltBuffer_load_uint8(state->tx_buffer, 0xD2);
        BoltBuffer_load_int32_be(state->tx_buffer, size);
        BoltBuffer_load(state->tx_buffer, string, size);
    }
    return 0;
}

int _load_list_header(struct BoltConnection* connection, int32_t size)
{
    if (size < 0)
    {
        return -1;
    }
    struct BoltProtocolV1State* state = BoltProtocolV1_state(connection);
    if (size < 0x10)
    {
        BoltBuffer_load_uint8(state->tx_buffer, (uint8_t)(0x90 + size));
    }
    else if (size < 0x100)
    {
        BoltBuffer_load_uint8(state->tx_buffer, 0xD4);
        BoltBuffer_load_uint8(state->tx_buffer, (uint8_t)(size));
    }
    else if (size < 0x10000)
    {
        BoltBuffer_load_uint8(state->tx_buffer, 0xD5);
        BoltBuffer_load_uint16_be(state->tx_buffer, (uint16_t)(size));
    }
    else
    {
        BoltBuffer_load_uint8(state->tx_buffer, 0xD6);
        BoltBuffer_load_int32_be(state->tx_buffer, size);
    }
    return 0;
}

int _load_map_header(struct BoltConnection* connection, int32_t size)
{
    if (size < 0)
    {
        return -1;
    }
    struct BoltProtocolV1State* state = BoltProtocolV1_state(connection);
    if (size < 0x10)
    {
        BoltBuffer_load_uint8(state->tx_buffer, (uint8_t)(0xA0 + size));
    }
    else if (size < 0x100)
    {
        BoltBuffer_load_uint8(state->tx_buffer, 0xD8);
        BoltBuffer_load_uint8(state->tx_buffer, (uint8_t)(size));
    }
    else if (size < 0x10000)
    {
        BoltBuffer_load_uint8(state->tx_buffer, 0xD9);
        BoltBuffer_load_uint16_be(state->tx_buffer, (uint16_t)(size));
    }
    else
    {
        BoltBuffer_load_uint8(state->tx_buffer, 0xDA);
        BoltBuffer_load_int32_be(state->tx_buffer, size);
    }
    return 0;
}

int _load_structure_header(struct BoltConnection* connection, int16_t code, int8_t size)
{
    if (code < 0 || size < 0 || size >= 0x10)
    {
        return -1;
    }
    struct BoltProtocolV1State* state = BoltProtocolV1_state(connection);
    BoltBuffer_load_uint8(state->tx_buffer, (uint8_t)(0xB0 + size));
    BoltBuffer_load_int8(state->tx_buffer, code);
    return 0;
}

/**
 * Copy request data from buffer 1 to buffer 0, also adding chunks.
 *
 * @param connection
 * @return request ID
 */
int _enqueue(struct BoltConnection* connection)
{
    // TODO: more chunks if size is too big
    struct BoltProtocolV1State* state = BoltProtocolV1_state(connection);
    int size = BoltBuffer_unloadable(state->tx_buffer);
    char header[2];
    header[0] = (char)(size >> 8);
    header[1] = (char)(size);
    BoltBuffer_load(connection->tx_buffer, &header[0], sizeof(header));
    BoltBuffer_load(connection->tx_buffer, BoltBuffer_unload_target(state->tx_buffer, size), size);
    header[0] = (char)(0);
    header[1] = (char)(0);
    BoltBuffer_load(connection->tx_buffer, &header[0], sizeof(header));
    BoltBuffer_compact(state->tx_buffer);
    int request_id = state->next_request_id;
    state->next_request_id += 1;
    if (state->next_request_id < 0)
    {
        state->next_request_id = 0;
    }
    return request_id;
}

int _load_request(struct BoltConnection* connection, struct BoltValue* value)
{
    assert(BoltValue_type(value) == BOLT_REQUEST);
    try(_load_structure_header(connection, BoltRequest_code(value), value->size));
    for (int32_t i = 0; i < value->size; i++)
    {
        try(BoltProtocolV1_load(connection, BoltRequest_value(value, i)));
    }
    return _enqueue(connection);
}

int BoltProtocolV1_load_request(struct BoltConnection* connection, struct BoltValue* value)
{
    struct BoltProtocolV1State* state = BoltProtocolV1_state(connection);
    BoltLog_request(state->next_request_id, value, connection->protocol_version);
    return _load_request(connection, value);
}

int BoltProtocolV1_load_request_quietly(struct BoltConnection* connection, struct BoltValue* value)
{
    return _load_request(connection, value);
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
        case BOLT_STRING8:
            return BoltProtocolV1_load_string(connection, BoltString8_get(value), value->size);
        case BOLT_STRING16:
            return -1;
        case BOLT_STRING8_ARRAY:
            return -1;
        case BOLT_STRING16_ARRAY:
            return -1;
        case BOLT_DICTIONARY8:
        {
            try(_load_map_header(connection, value->size));
            for (int32_t i = 0; i < value->size; i++)
            {
                const char * key = BoltDictionary8_get_key(value, i);
                if (key != NULL)
                {
                    try(BoltProtocolV1_load_string(connection, key, BoltDictionary8_get_key_size(value, i)));
                    try(BoltProtocolV1_load(connection, BoltDictionary8_value(value, i)));
                }
            }
            return 0;
        }
        case BOLT_DICTIONARY16:
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
            return BoltProtocolV1_load_request(connection, value);
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
            return -1;
    }
}

int BoltProtocolV1_compile_INIT(struct BoltValue* value, const char* user_agent, const char* user, const char* password)
{
    BoltValue_to_Request(value, 0x01, 2);
    BoltValue_to_String8(BoltRequest_value(value, 0), user_agent, strlen(user_agent));
    struct BoltValue* auth = BoltRequest_value(value, 1);
    if (user == NULL || password == NULL)
    {
        BoltValue_to_Dictionary8(auth, 0);
    }
    else
    {
        BoltValue_to_Dictionary8(auth, 3);
        BoltDictionary8_set_key(auth, 0, "scheme", 6);
        BoltDictionary8_set_key(auth, 1, "principal", 9);
        BoltDictionary8_set_key(auth, 2, "credentials", 11);
        BoltValue_to_String8(BoltDictionary8_value(auth, 0), "basic", 5);
        BoltValue_to_String8(BoltDictionary8_value(auth, 1), user, strlen(user));
        BoltValue_to_String8(BoltDictionary8_value(auth, 2), password, strlen(password));
    }
    return 0;
}

int _unload(struct BoltConnection* connection, struct BoltValue* value);

int _unload_null(struct BoltConnection* connection, struct BoltValue* value)
{
    struct BoltProtocolV1State* state = BoltProtocolV1_state(connection);
    uint8_t marker;
    BoltBuffer_unload_uint8(state->rx_buffer, &marker);
    if (marker == 0xC0)
    {
        BoltValue_to_Null(value);
    }
    else
    {
        return -1;  // BOLT_ERROR_WRONG_TYPE
    }
    return 0;
}

int _unload_boolean(struct BoltConnection* connection, struct BoltValue* value)
{
    struct BoltProtocolV1State* state = BoltProtocolV1_state(connection);
    uint8_t marker;
    BoltBuffer_unload_uint8(state->rx_buffer, &marker);
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
    return 0;
}

int _unload_integer(struct BoltConnection* connection, struct BoltValue* value)
{
    struct BoltProtocolV1State* state = BoltProtocolV1_state(connection);
    uint8_t marker;
    BoltBuffer_unload_uint8(state->rx_buffer, &marker);
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
        BoltBuffer_unload_int8(state->rx_buffer, &x);
        BoltValue_to_Int64(value, x);
    }
    else if (marker == 0xC9)
    {
        int16_t x;
        BoltBuffer_unload_int16_be(state->rx_buffer, &x);
        BoltValue_to_Int64(value, x);
    }
    else if (marker == 0xCA)
    {
        int32_t x;
        BoltBuffer_unload_int32_be(state->rx_buffer, &x);
        BoltValue_to_Int64(value, x);
    }
    else if (marker == 0xCB)
    {
        int64_t x;
        BoltBuffer_unload_int64_be(state->rx_buffer, &x);
        BoltValue_to_Int64(value, x);
    }
    else
    {
        return -1;  // BOLT_ERROR_WRONG_TYPE
    }
    return 0;
}

int _unload_float(struct BoltConnection* connection, struct BoltValue* value)
{
    struct BoltProtocolV1State* state = BoltProtocolV1_state(connection);
    uint8_t marker;
    BoltBuffer_unload_uint8(state->rx_buffer, &marker);
    if (marker == 0xC1)
    {
        double x;
        BoltBuffer_unload_double_be(state->rx_buffer, &x);
        BoltValue_to_Float64(value, x);
    }
    else
    {
        return -1;  // BOLT_ERROR_WRONG_TYPE
    }
    return 0;
}

int _unload_string(struct BoltConnection* connection, struct BoltValue* value)
{
    struct BoltProtocolV1State* state = BoltProtocolV1_state(connection);
    uint8_t marker;
    BoltBuffer_unload_uint8(state->rx_buffer, &marker);
    if (marker >= 0x80 && marker <= 0x8F)
    {
        int32_t size;
        size = marker & 0x0F;
        BoltValue_to_String8(value, NULL, size);
        BoltBuffer_unload(state->rx_buffer, BoltString8_get(value), size);
        return 0;
    }
    if (marker == 0xD0)
    {
        uint8_t size;
        BoltBuffer_unload_uint8(state->rx_buffer, &size);
        BoltValue_to_String8(value, NULL, size);
        BoltBuffer_unload(state->rx_buffer, BoltString8_get(value), size);
        return 0;
    }
    if (marker == 0xD1)
    {
        uint16_t size;
        BoltBuffer_unload_uint16_be(state->rx_buffer, &size);
        BoltValue_to_String8(value, NULL, size);
        BoltBuffer_unload(state->rx_buffer, BoltString8_get(value), size);
        return 0;
    }
    if (marker == 0xD2)
    {
        int32_t size;
        BoltBuffer_unload_int32_be(state->rx_buffer, &size);
        BoltValue_to_String8(value, NULL, size);
        BoltBuffer_unload(state->rx_buffer, BoltString8_get(value), size);
        return 0;
    }
    BoltLog_error("[NET] Wrong marker type: %d", marker);
    return -1;  // BOLT_ERROR_WRONG_TYPE
}

int _unload_list(struct BoltConnection* connection, struct BoltValue* value)
{
    struct BoltProtocolV1State* state = BoltProtocolV1_state(connection);
    uint8_t marker;
    int32_t size;
    BoltBuffer_unload_uint8(state->rx_buffer, &marker);
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
    struct BoltProtocolV1State* state = BoltProtocolV1_state(connection);
    uint8_t marker;
    int32_t size;
    BoltBuffer_unload_uint8(state->rx_buffer, &marker);
    if (marker >= 0xA0 && marker <= 0xAF)
    {
        size = marker & 0x0F;
        BoltValue_to_Dictionary8(value, size);
        for (int i = 0; i < size; i++)
        {
            _unload(connection, BoltDictionary8_key(value, i));
            _unload(connection, BoltDictionary8_value(value, i));
        }
        return 0;
    }
    // TODO: bigger maps
    return -1;  // BOLT_ERROR_WRONG_TYPE
}

int _unload_structure(struct BoltConnection* connection, struct BoltValue* value)
{
    struct BoltProtocolV1State* state = BoltProtocolV1_state(connection);
    uint8_t marker;
    int8_t code;
    int32_t size;
    BoltBuffer_unload_uint8(state->rx_buffer, &marker);
    if (marker >= 0xB0 && marker <= 0xBF)
    {
        size = marker & 0x0F;
        BoltBuffer_unload_int8(state->rx_buffer, &code);
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
    struct BoltProtocolV1State* state = BoltProtocolV1_state(connection);
    uint8_t marker;
    BoltBuffer_peek_uint8(state->rx_buffer, &marker);
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

int BoltProtocolV1_unload(struct BoltConnection* connection)
{
    struct BoltProtocolV1State* state = BoltProtocolV1_state(connection);
    if (BoltBuffer_unloadable(state->rx_buffer) == 0)
    {
        return 0;
    }
    uint8_t marker;
    uint8_t code;
    int32_t size;
    BoltBuffer_unload_uint8(state->rx_buffer, &marker);
    if (BoltProtocolV1_marker_type(marker) != BOLT_V1_STRUCTURE)
    {
        return -1;
    }
    size = marker & 0x0F;
    struct BoltValue* received = ((struct BoltProtocolV1State*)(connection->protocol_state))->fetched;
    BoltBuffer_unload_uint8(state->rx_buffer, &code);
    if (code == 0x71)
    {
        // Record
        if (size >= 1)
        {
            _unload(connection, received);
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
            BoltValue_to_Null(received);
        }
    }
    else
    {
        // Summary
        BoltValue_to_Summary(received, code, size);
        for (int i = 0; i < size; i++)
        {
            _unload(connection, BoltSummary_value(received, i));
        }
        BoltLog_summary(state->response_counter, received, connection->protocol_version);
    }
    return 1;
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
