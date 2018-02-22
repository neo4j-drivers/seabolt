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
#include <bolt/values.h>
#include <memory.h>
#include <assert.h>
#include "bolt/buffering.h"
#include "v1.h"
#include "bolt/mem.h"
#include "bolt/logging.h"

#define ACK_FAILURE 0x0E
#define RESET       0x0F
#define RUN         0x10
#define DISCARD_ALL 0x2F
#define PULL_ALL    0x3F

#define INITIAL_TX_BUFFER_SIZE 8192
#define INITIAL_RX_BUFFER_SIZE 8192

#define MAX_BOOKMARK_SIZE 40
#define MAX_SERVER_SIZE 200

#define MAX_LOGGED_RECORDS 3

#define char_to_uint16be(array) ((uint8_t)(header[0]) << 8) | (uint8_t)(header[1]);

#define TRY(code) { int status = (code); if (status == -1) { return status; } }


int BoltProtocolV1_compile_INIT(struct BoltValue* value, const char* user_agent, const char* user, const char* password)
{
    BoltValue_to_Message(value, 0x01, 2);
    BoltValue_to_String(BoltMessage_value(value, 0), user_agent, strlen(user_agent));
    struct BoltValue* auth = BoltMessage_value(value, 1);
    if (user == NULL || password == NULL)
    {
        BoltValue_to_Dictionary(auth, 0);
    }
    else
    {
        BoltValue_to_Dictionary(auth, 3);
        BoltDictionary_set_key(auth, 0, "scheme", 6);
        BoltDictionary_set_key(auth, 1, "principal", 9);
        BoltDictionary_set_key(auth, 2, "credentials", 11);
        BoltValue_to_String(BoltDictionary_value(auth, 0), "basic", 5);
        BoltValue_to_String(BoltDictionary_value(auth, 1), user, strlen(user));
        BoltValue_to_String(BoltDictionary_value(auth, 2), password, strlen(password));
    }
    return 0;
}

void compile_RUN(struct _run_request * run, int32_t n_parameters)
{
    run->request = BoltValue_create();
    BoltValue_to_Message(run->request, RUN, 2);
    run->statement = BoltMessage_value(run->request, 0);
    run->parameters = BoltMessage_value(run->request, 1);
    BoltValue_to_Dictionary(run->parameters, n_parameters);
}

struct BoltProtocolV1State* BoltProtocolV1_create_state()
{
    struct BoltProtocolV1State* state = BoltMem_allocate(sizeof(struct BoltProtocolV1State));

    state->tx_buffer = BoltBuffer_create(INITIAL_TX_BUFFER_SIZE);
    state->rx_buffer = BoltBuffer_create(INITIAL_RX_BUFFER_SIZE);

    state->server = BoltMem_allocate(MAX_SERVER_SIZE);
    memset(state->server, 0, MAX_SERVER_SIZE);
    state->fields = BoltValue_create();
    state->last_bookmark = BoltMem_allocate(MAX_BOOKMARK_SIZE);
    memset(state->last_bookmark, 0, MAX_BOOKMARK_SIZE);

    state->next_request_id = 0;
    state->response_counter = 0;

    compile_RUN(&state->run, 0);
    compile_RUN(&state->begin, 0);
    BoltValue_to_String(state->begin.statement, "BEGIN", 5);
    compile_RUN(&state->commit, 0);
    BoltValue_to_String(state->commit.statement, "COMMIT", 6);
    compile_RUN(&state->rollback, 0);
    BoltValue_to_String(state->rollback.statement, "ROLLBACK", 8);

    state->discard_request = BoltValue_create();
    BoltValue_to_Message(state->discard_request, DISCARD_ALL, 0);

    state->pull_request = BoltValue_create();
    BoltValue_to_Message(state->pull_request, PULL_ALL, 0);

    state->reset_request = BoltValue_create();
    BoltValue_to_Message(state->reset_request, RESET, 0);

    state->data = BoltValue_create();
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

    BoltValue_destroy(state->reset_request);

    BoltMem_deallocate(state->server, MAX_SERVER_SIZE);
    BoltValue_destroy(state->fields);
    BoltMem_deallocate(state->last_bookmark, MAX_BOOKMARK_SIZE);

    BoltValue_destroy(state->data);

    BoltMem_deallocate(state, sizeof(struct BoltProtocolV1State));
}

struct BoltProtocolV1State* BoltProtocolV1_state(struct BoltConnection* connection)
{
    return (struct BoltProtocolV1State*)(connection->protocol_state);
}

enum BoltProtocolV1Type marker_type(uint8_t marker)
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

int load(struct BoltBuffer * buffer, struct BoltValue * value);

/**
 * Copy request data from buffer 1 to buffer 0, also adding chunks.
 *
 * @param connection
 */
void enqueue(struct BoltConnection * connection);

int load_null(struct BoltBuffer * buffer)
{
    BoltBuffer_load_uint8(buffer, 0xC0);
    return 0;
}

int load_boolean(struct BoltBuffer * buffer, int value)
{
    BoltBuffer_load_uint8(buffer, (value == 0) ? (uint8_t)(0xC2) : (uint8_t)(0xC3));
    return 0;
}

int load_integer(struct BoltBuffer * buffer, int64_t value)
{
    if (value >= -0x10 && value < 0x80)
    {
        BoltBuffer_load_int8(buffer, (int8_t)(value));
    }
    else if (value >= INT8_MIN && value <= INT8_MAX)
    {
        BoltBuffer_load_uint8(buffer, 0xC8);
        BoltBuffer_load_int8(buffer, (int8_t)(value));
    }
    else if (value >= INT16_MIN && value <= INT16_MAX)
    {
        BoltBuffer_load_uint8(buffer, 0xC9);
        BoltBuffer_load_int16_be(buffer, (int16_t)(value));
    }
    else if (value >= INT32_MIN && value <= INT32_MAX)
    {
        BoltBuffer_load_uint8(buffer, 0xCA);
        BoltBuffer_load_int32_be(buffer, (int32_t)(value));
    }
    else
    {
        BoltBuffer_load_uint8(buffer, 0xCB);
        BoltBuffer_load_int64_be(buffer, value);
    }
    return 0;
}

int load_float(struct BoltBuffer * buffer, double value)
{
    BoltBuffer_load_uint8(buffer, 0xC1);
    BoltBuffer_load_double_be(buffer, value);
    return 0;
}

int load_bytes(struct BoltBuffer * buffer, const char * string, int32_t size)
{
    if (size < 0)
    {
        return -1;
    }
    if (size < 0x100)
    {
        BoltBuffer_load_uint8(buffer, 0xCC);
        BoltBuffer_load_uint8(buffer, (uint8_t)(size));
        BoltBuffer_load(buffer, string, size);
    }
    else if (size < 0x10000)
    {
        BoltBuffer_load_uint8(buffer, 0xCD);
        BoltBuffer_load_uint16_be(buffer, (uint16_t)(size));
        BoltBuffer_load(buffer, string, size);
    }
    else
    {
        BoltBuffer_load_uint8(buffer, 0xCE);
        BoltBuffer_load_int32_be(buffer, size);
        BoltBuffer_load(buffer, string, size);
    }
    return 0;
}

int load_string_header(struct BoltBuffer * buffer, int32_t size)
{
    if (size < 0)
    {
        return -1;
    }
    if (size < 0x10)
    {
        BoltBuffer_load_uint8(buffer, (uint8_t)(0x80 + size));
    }
    else if (size < 0x100)
    {
        BoltBuffer_load_uint8(buffer, 0xD0);
        BoltBuffer_load_uint8(buffer, (uint8_t)(size));
    }
    else if (size < 0x10000)
    {
        BoltBuffer_load_uint8(buffer, 0xD1);
        BoltBuffer_load_uint16_be(buffer, (uint16_t)(size));
    }
    else
    {
        BoltBuffer_load_uint8(buffer, 0xD2);
        BoltBuffer_load_int32_be(buffer, size);
    }
    return 0;
}

int load_string(struct BoltBuffer * buffer, const char * string, int32_t size)
{
    int status = load_string_header(buffer, size);
    if (status < 0) return status;
    BoltBuffer_load(buffer, string, size);
    return 0;
}

int load_string_from_char(struct BoltBuffer * buffer, uint32_t ch)
{
    int ch_size = BoltBuffer_sizeof_utf8_char(ch);
    load_string_header(buffer, ch_size);
    BoltBuffer_load_utf8_char(buffer, ch);
    return ch_size;
}

int load_list_header(struct BoltBuffer * buffer, int32_t size)
{
    if (size < 0)
    {
        return -1;
    }
    if (size < 0x10)
    {
        BoltBuffer_load_uint8(buffer, (uint8_t)(0x90 + size));
    }
    else if (size < 0x100)
    {
        BoltBuffer_load_uint8(buffer, 0xD4);
        BoltBuffer_load_uint8(buffer, (uint8_t)(size));
    }
    else if (size < 0x10000)
    {
        BoltBuffer_load_uint8(buffer, 0xD5);
        BoltBuffer_load_uint16_be(buffer, (uint16_t)(size));
    }
    else
    {
        BoltBuffer_load_uint8(buffer, 0xD6);
        BoltBuffer_load_int32_be(buffer, size);
    }
    return 0;
}

int load_list_of_strings_from_char_array(struct BoltBuffer * buffer, const uint32_t * array, int32_t size)
{
    load_list_header(buffer, size);
    for (int32_t i = 0; i < size; i++)
    {
        int ch_size = BoltBuffer_sizeof_utf8_char(array[i]);
        load_string_header(buffer, ch_size);
        BoltBuffer_load_utf8_char(buffer, array[i]);
    }
    return 0;
}

int load_map_header(struct BoltBuffer * buffer, int32_t size)
{
    if (size < 0)
    {
        return -1;
    }
    if (size < 0x10)
    {
        BoltBuffer_load_uint8(buffer, (uint8_t)(0xA0 + size));
    }
    else if (size < 0x100)
    {
        BoltBuffer_load_uint8(buffer, 0xD8);
        BoltBuffer_load_uint8(buffer, (uint8_t)(size));
    }
    else if (size < 0x10000)
    {
        BoltBuffer_load_uint8(buffer, 0xD9);
        BoltBuffer_load_uint16_be(buffer, (uint16_t)(size));
    }
    else
    {
        BoltBuffer_load_uint8(buffer, 0xDA);
        BoltBuffer_load_int32_be(buffer, size);
    }
    return 0;
}

int load_structure_header(struct BoltBuffer * buffer, int16_t code, int8_t size)
{
    if (code < 0 || size < 0 || size >= 0x10)
    {
        return -1;
    }
    BoltBuffer_load_uint8(buffer, (uint8_t)(0xB0 + size));
    BoltBuffer_load_int8(buffer, (int8_t)(code));
    return 0;
}

int load_message(struct BoltConnection * connection, struct BoltValue * value)
{
    assert(BoltValue_type(value) == BOLT_MESSAGE);
    struct BoltProtocolV1State* state = BoltProtocolV1_state(connection);
    TRY(load_structure_header(state->tx_buffer, BoltMessage_code(value), value->size));
    for (int32_t i = 0; i < value->size; i++)
    {
        TRY(load(state->tx_buffer, BoltMessage_value(value, i)));
    }
    enqueue(connection);
    return 0;
}

int BoltProtocolV1_load_message(struct BoltConnection * connection, struct BoltValue * value)
{
    struct BoltProtocolV1State* state = BoltProtocolV1_state(connection);
    BoltLog_message("C", state->next_request_id, value, connection->protocol_version);
    return load_message(connection, value);
}

int BoltProtocolV1_load_message_quietly(struct BoltConnection * connection, struct BoltValue * value)
{
    return load_message(connection, value);
}

#define LOAD_LIST_FROM_INT_ARRAY(connection, value, type)                               \
{                                                                                       \
    TRY(load_list_header(connection, (value)->size));                                   \
    for (int32_t i = 0; i < (value)->size; i++)                                         \
    {                                                                                   \
        TRY(load_integer(connection, Bolt##type##Array_get(value, i)));                 \
    }                                                                                   \
}                                                                                       \

int load(struct BoltBuffer * buffer, struct BoltValue * value)
{
    switch (BoltValue_type(value))
    {
        case BOLT_NULL:
            return load_null(buffer);
        case BOLT_LIST:
        {
            TRY(load_list_header(buffer, value->size));
            for (int32_t i = 0; i < value->size; i++)
            {
                TRY(load(buffer, BoltList_value(value, i)));
            }
            return 0;
        }
        case BOLT_BIT:
            return load_boolean(buffer, BoltBit_get(value));
        case BOLT_BIT_ARRAY:
        {
            TRY(load_list_header(buffer, value->size));
            for (int32_t i = 0; i < value->size; i++)
            {
                TRY(load_boolean(buffer, BoltBitArray_get(value, i)));
            }
            return 0;
        }
        case BOLT_BYTE:
            // A Byte is coerced to an Integer (Int64)
            return load_integer(buffer, BoltByte_get(value));
        case BOLT_BYTE_ARRAY:
            return load_bytes(buffer, BoltByteArray_get_all(value), value->size);
        case BOLT_CHAR:
            return load_string_from_char(buffer, BoltChar_get(value));
        case BOLT_CHAR_ARRAY:
            return load_list_of_strings_from_char_array(buffer, BoltCharArray_get(value), value->size);
        case BOLT_STRING:
            return load_string(buffer, BoltString_get(value), value->size);
        case BOLT_STRING_ARRAY:
        {
            load_list_header(buffer, value->size);
            for (int32_t i = 0; i < value->size; i++)
            {
                int string_size = BoltStringArray_get_size(value, i);
                load_string_header(buffer, string_size);
                BoltBuffer_load(buffer, BoltStringArray_get(value, i), string_size);
            }
            return 0;
        }
        case BOLT_DICTIONARY:
        {
            TRY(load_map_header(buffer, value->size));
            for (int32_t i = 0; i < value->size; i++)
            {
                const char * key = BoltDictionary_get_key(value, i);
                if (key != NULL)
                {
                    TRY(load_string(buffer, key, BoltDictionary_get_key_size(value, i)));
                    TRY(load(buffer, BoltDictionary_value(value, i)));
                }
            }
            return 0;
        }
        case BOLT_INT16:
            return load_integer(buffer, BoltInt16_get(value));
        case BOLT_INT32:
            return load_integer(buffer, BoltInt32_get(value));
        case BOLT_INT64:
            return load_integer(buffer, BoltInt64_get(value));
        case BOLT_INT16_ARRAY:
            LOAD_LIST_FROM_INT_ARRAY(buffer, value, Int16);
            return 0;
        case BOLT_INT32_ARRAY:
            LOAD_LIST_FROM_INT_ARRAY(buffer, value, Int32);
            return 0;
        case BOLT_INT64_ARRAY:
            LOAD_LIST_FROM_INT_ARRAY(buffer, value, Int64);
            return 0;
        case BOLT_FLOAT64:
            return load_float(buffer, BoltFloat64_get(value));
        case BOLT_FLOAT64_ARRAY:
        {
            TRY(load_list_header(buffer, value->size));
            for (int32_t i = 0; i < value->size; i++)
            {
                TRY(load_float(buffer, BoltFloat64Array_get(value, i)));
            }
            return 0;
        }
        case BOLT_STRUCTURE:
        {
            TRY(load_structure_header(buffer, BoltStructure_code(value), value->size));
            for (int32_t i = 0; i < value->size; i++)
            {
                TRY(load(buffer, BoltStructure_value(value, i)));
            }
            return 0;
        }
        case BOLT_STRUCTURE_ARRAY:
            return -1;
        case BOLT_MESSAGE:
            assert(0);
            return -1;
//            return BoltProtocolV1_load_message(connection, value);
        default:
            // TODO:  TYPE_NOT_SUPPORTED (vs TYPE_NOT_IMPLEMENTED)
            assert(0);
            return -1;
    }
}

/**
 * Copy request data from buffer 1 to buffer 0, also adding chunks.
 *
 * @param connection
 * @return request ID
 */
void enqueue(struct BoltConnection * connection)
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
    state->next_request_id += 1;
}

int unload(struct BoltConnection * connection, struct BoltValue * value);

int unload_null(struct BoltConnection * connection, struct BoltValue * value)
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

int unload_boolean(struct BoltConnection * connection, struct BoltValue * value)
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

int unload_integer(struct BoltConnection * connection, struct BoltValue * value)
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

int unload_float(struct BoltConnection * connection, struct BoltValue * value)
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

int unload_string(struct BoltConnection * connection, struct BoltValue * value)
{
    struct BoltProtocolV1State* state = BoltProtocolV1_state(connection);
    uint8_t marker;
    BoltBuffer_unload_uint8(state->rx_buffer, &marker);
    if (marker >= 0x80 && marker <= 0x8F)
    {
        int32_t size;
        size = marker & 0x0F;
        BoltValue_to_String(value, NULL, size);
        BoltBuffer_unload(state->rx_buffer, BoltString_get(value), size);
        return 0;
    }
    if (marker == 0xD0)
    {
        uint8_t size;
        BoltBuffer_unload_uint8(state->rx_buffer, &size);
        BoltValue_to_String(value, NULL, size);
        BoltBuffer_unload(state->rx_buffer, BoltString_get(value), size);
        return 0;
    }
    if (marker == 0xD1)
    {
        uint16_t size;
        BoltBuffer_unload_uint16_be(state->rx_buffer, &size);
        BoltValue_to_String(value, NULL, size);
        BoltBuffer_unload(state->rx_buffer, BoltString_get(value), size);
        return 0;
    }
    if (marker == 0xD2)
    {
        int32_t size;
        BoltBuffer_unload_int32_be(state->rx_buffer, &size);
        BoltValue_to_String(value, NULL, size);
        BoltBuffer_unload(state->rx_buffer, BoltString_get(value), size);
        return 0;
    }
    BoltLog_error("bolt: Unknown marker: %d", marker);
    return -1;  // BOLT_ERROR_WRONG_TYPE
}

int unload_bytes(struct BoltConnection * connection, struct BoltValue * value)
{
    struct BoltProtocolV1State* state = BoltProtocolV1_state(connection);
    uint8_t marker;
    BoltBuffer_unload_uint8(state->rx_buffer, &marker);
    if (marker == 0xCC)
    {
        uint8_t size;
        BoltBuffer_unload_uint8(state->rx_buffer, &size);
        BoltValue_to_ByteArray(value, NULL, size);
        BoltBuffer_unload(state->rx_buffer, BoltByteArray_get_all(value), size);
        return 0;
    }
    if (marker == 0xCD)
    {
        uint16_t size;
        BoltBuffer_unload_uint16_be(state->rx_buffer, &size);
        BoltValue_to_ByteArray(value, NULL, size);
        BoltBuffer_unload(state->rx_buffer, BoltByteArray_get_all(value), size);
        return 0;
    }
    if (marker == 0xCE)
    {
        int32_t size;
        BoltBuffer_unload_int32_be(state->rx_buffer, &size);
        BoltValue_to_ByteArray(value, NULL, size);
        BoltBuffer_unload(state->rx_buffer, BoltByteArray_get_all(value), size);
        return 0;
    }
    BoltLog_error("bolt: Unknown marker: %d", marker);
    return -1;  // BOLT_ERROR_WRONG_TYPE
}

int unload_list(struct BoltConnection * connection, struct BoltValue * value)
{
    struct BoltProtocolV1State* state = BoltProtocolV1_state(connection);
    uint8_t marker;
    int32_t size;
    BoltBuffer_unload_uint8(state->rx_buffer, &marker);
    if (marker >= 0x90 && marker <= 0x9F)
    {
        size = marker & 0x0F;
    }
    else if (marker == 0xD4)
    {
        uint8_t size_;
        BoltBuffer_unload_uint8(state->rx_buffer, &size_);
        size = size_;
    }
    else if (marker == 0xD5)
    {
        uint16_t size_;
        BoltBuffer_unload_uint16_be(state->rx_buffer, &size_);
        size = size_;
    }
    else if (marker == 0xD6)
    {
        int32_t size_;
        BoltBuffer_unload_int32_be(state->rx_buffer, &size_);
        size = size_;
    }
    else
    {
        return -1;  // BOLT_ERROR_WRONG_TYPE
    }
    if (size < 0)
    {
        return -1;  // invalid size
    }
    BoltValue_to_List(value, size);
    for (int i = 0; i < size; i++)
    {
        unload(connection, BoltList_value(value, i));
    }
    return size;
}

int unload_map(struct BoltConnection * connection, struct BoltValue * value)
{
    struct BoltProtocolV1State* state = BoltProtocolV1_state(connection);
    uint8_t marker;
    int32_t size;
    BoltBuffer_unload_uint8(state->rx_buffer, &marker);
    if (marker >= 0xA0 && marker <= 0xAF)
    {
        size = marker & 0x0F;
    }
    else if (marker == 0xD8)
    {
        uint8_t size_;
        BoltBuffer_unload_uint8(state->rx_buffer, &size_);
        size = size_;
    }
    else if (marker == 0xD9)
    {
        uint16_t size_;
        BoltBuffer_unload_uint16_be(state->rx_buffer, &size_);
        size = size_;
    }
    else if (marker == 0xDA)
    {
        int32_t size_;
        BoltBuffer_unload_int32_be(state->rx_buffer, &size_);
        size = size_;
    }
    else
    {
        return -1;  // BOLT_ERROR_WRONG_TYPE
    }
    if (size < 0)
    {
        return -1;  // invalid size
    }
    BoltValue_to_Dictionary(value, size);
    for (int i = 0; i < size; i++)
    {
        unload(connection, BoltDictionary_key(value, i));
        unload(connection, BoltDictionary_value(value, i));
    }
    return size;
}

int unload_structure(struct BoltConnection * connection, struct BoltValue * value)
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
            unload(connection, BoltStructure_value(value, i));
        }
        return 0;
    }
    // TODO: bigger structures (that are never actually used)
    return -1;  // BOLT_ERROR_WRONG_TYPE
}

int unload(struct BoltConnection * connection, struct BoltValue * value)
{
    struct BoltProtocolV1State* state = BoltProtocolV1_state(connection);
    uint8_t marker;
    BoltBuffer_peek_uint8(state->rx_buffer, &marker);
    switch(marker_type(marker))
    {
        case BOLT_V1_NULL:
            return unload_null(connection, value);
        case BOLT_V1_BOOLEAN:
            return unload_boolean(connection, value);
        case BOLT_V1_INTEGER:
            return unload_integer(connection, value);
        case BOLT_V1_FLOAT:
            return unload_float(connection, value);
        case BOLT_V1_STRING:
            return unload_string(connection, value);
        case BOLT_V1_BYTES:
            return unload_bytes(connection, value);
        case BOLT_V1_LIST:
            return unload_list(connection, value);
        case BOLT_V1_MAP:
            return unload_map(connection, value);
        case BOLT_V1_STRUCTURE:
            return unload_structure(connection, value);
        default:
            BoltLog_error("bolt: Unknown marker: %d", marker);
            return -1;  // BOLT_UNSUPPORTED_MARKER
    }
}

int BoltProtocolV1_fetch_b(struct BoltConnection * connection, bolt_request_t request_id)
{
    struct BoltProtocolV1State * state = BoltProtocolV1_state(connection);
    bolt_request_t response_id;
    do
    {
        char header[2];
        int fetched = BoltConnection_receive_b(connection, &header[0], 2);
        if (fetched == -1)
        {
            BoltLog_error("bolt: Could not fetch chunk header");
            return -1;
        }
        uint16_t chunk_size = char_to_uint16be(header);
        BoltBuffer_compact(state->rx_buffer);
        while (chunk_size != 0)
        {
            fetched = BoltConnection_receive_b(connection, BoltBuffer_load_target(state->rx_buffer, chunk_size),
                                               chunk_size);
            if (fetched == -1)
            {
                BoltLog_error("bolt: Could not fetch chunk data");
                return -1;
            }
            fetched = BoltConnection_receive_b(connection, &header[0], 2);
            if (fetched == -1)
            {
                BoltLog_error("bolt: Could not fetch chunk header");
                return -1;
            }
            chunk_size = char_to_uint16be(header);
        }
        response_id = state->response_counter;
        BoltProtocolV1_unload(connection);
        if (BoltValue_type(state->data) == BOLT_MESSAGE)
        {
            state->response_counter += 1;
        }
    } while (response_id != request_id);
    if (BoltValue_type(state->data) == BOLT_MESSAGE)
    {
        BoltProtocolV1_extract_metadata(connection, state->data);
        return 0;
    }
    return 1;
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
    if (marker_type(marker) != BOLT_V1_STRUCTURE)
    {
        return -1;
    }
    size = marker & 0x0F;
    struct BoltValue* received = ((struct BoltProtocolV1State*)(connection->protocol_state))->data;
    BoltBuffer_unload_uint8(state->rx_buffer, &code);
    if (code == BOLT_V1_RECORD)
    {
        if (size >= 1)
        {
            unload(connection, received);
            if (size > 1)
            {
                struct BoltValue* black_hole = BoltValue_create();
                for (int i = 1; i < size; i++)
                {
                    unload(connection, black_hole);
                }
                BoltValue_destroy(black_hole);
            }
        }
        else
        {
            BoltValue_to_Null(received);
        }
        if (state->record_counter < MAX_LOGGED_RECORDS)
        {
            BoltLog_message("S", state->response_counter, state->data, connection->protocol_version);
        }
        state->record_counter += 1;
    }
    else /* Summary */
    {
        BoltValue_to_Message(received, code, size);
        for (int i = 0; i < size; i++)
        {
            unload(connection, BoltMessage_value(received, i));
        }
        if (state->record_counter > MAX_LOGGED_RECORDS)
        {
            BoltLog_info("bolt: S[%d]: Received %llu more records", state->response_counter, state->record_counter - MAX_LOGGED_RECORDS);
        }
        state->record_counter = 0;
        BoltLog_message("S", state->response_counter, received, connection->protocol_version);
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
            return "?";
    }
}

const char* BoltProtocolV1_message_name(int16_t code)
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
        case 0x70:
            return "SUCCESS";
        case 0x7E:
            return "IGNORED";
        case 0x7F:
            return "FAILURE";
        default:
            return "?";
    }
}

int BoltProtocolV1_init_b(struct BoltConnection * connection, const char * user_agent,
                          const char * user, const char * password)
{
    struct BoltValue * init = BoltValue_create();
    BoltProtocolV1_compile_INIT(init, user_agent, user, "*******");
    struct BoltProtocolV1State * state = BoltProtocolV1_state(connection);
    BoltLog_message("C", state->next_request_id, init, connection->protocol_version);
    BoltProtocolV1_compile_INIT(init, user_agent, user, password);
    BoltProtocolV1_load_message_quietly(connection, init);
    bolt_request_t init_request = BoltConnection_last_request(connection);
    BoltValue_destroy(init);
    TRY(BoltConnection_send_b(connection));
    BoltConnection_fetch_summary_b(connection, init_request);
    return BoltMessage_code(BoltConnection_data(connection));
}

int BoltProtocolV1_reset_b(struct BoltConnection * connection)
{
    struct BoltProtocolV1State * state = BoltProtocolV1_state(connection);
    BoltProtocolV1_load_message(connection, state->reset_request);
    bolt_request_t reset_request = BoltConnection_last_request(connection);
    TRY(BoltConnection_send_b(connection));
    BoltConnection_fetch_summary_b(connection, reset_request);
    return BoltMessage_code(BoltConnection_data(connection));
}

void BoltProtocolV1_extract_metadata(struct BoltConnection * connection, struct BoltValue * summary)
{
    struct BoltProtocolV1State* state = BoltProtocolV1_state(connection);
    if (summary->size >= 1)
    {
        struct BoltValue * metadata = BoltMessage_value(summary, 0);
        switch (BoltValue_type(metadata))
        {
            case BOLT_DICTIONARY:
            {
                for (int32_t i = 0; i < metadata->size; i++)
                {
                    const char * key = BoltDictionary_get_key(metadata, i);
                    if (strcmp(key, "bookmark") == 0)
                    {
                        struct BoltValue * value = BoltDictionary_value(metadata, i);
                        switch (BoltValue_type(value))
                        {
                            case BOLT_STRING:
                            {
                                memset(state->last_bookmark, 0, MAX_BOOKMARK_SIZE);
                                memcpy(state->last_bookmark, BoltString_get(value), (size_t)(value->size));
                                BoltLog_info("bolt: <SET last_bookmark=\"%s\">", state->last_bookmark);
                                break;
                            }
                            default:
                                break;
                        }
                    }
                    else if (strcmp(key, "fields") == 0)
                    {
                        struct BoltValue * value = BoltDictionary_value(metadata, i);
                        switch (BoltValue_type(value))
                        {
                            case BOLT_LIST:
                            {
                                struct BoltValue * target_value = state->fields;
                                BoltValue_to_StringArray(target_value, value->size);
                                for (int j = 0; j < value->size; j++)
                                {
                                    struct BoltValue * source_value = BoltList_value(value, j);
                                    switch (BoltValue_type(source_value))
                                    {
                                        case BOLT_STRING:
                                            BoltStringArray_put(target_value, j, BoltString_get(source_value), source_value->size);
                                            break;
                                        default:
                                            BoltStringArray_put(target_value, j, "?", 1);
                                    }
                                }
                                BoltLog_value(target_value, 1, "<SET fields=", ">");
                                break;
                            }
                            default:
                                break;
                        }
                    }
                    else if (strcmp(key, "server") == 0)
                    {
                        struct BoltValue * value = BoltDictionary_value(metadata, i);
                        switch (BoltValue_type(value))
                        {
                            case BOLT_STRING:
                            {
                                memset(state->server, 0, MAX_SERVER_SIZE);
                                memcpy(state->server, BoltString_get(value), (size_t)(value->size));
                                BoltLog_info("bolt: <SET server=\"%s\">", state->server);
                                break;
                            }
                            default:
                                break;
                        }
                    }
                }
                break;
            }
            default:
                break;
        }
    }
}

int BoltProtocolV1_set_cypher_template(struct BoltConnection * connection, const char * statement, size_t size)
{
    if (size <= INT32_MAX)
    {
        struct BoltProtocolV1State * state = BoltProtocolV1_state(connection);
        BoltValue_to_String(state->run.statement, statement, (int32_t)(size));
        return 0;
    }
    return -1;
}

int BoltProtocolV1_set_n_cypher_parameters(struct BoltConnection * connection, int32_t size)
{
    struct BoltProtocolV1State * state = BoltProtocolV1_state(connection);
    BoltValue_to_Dictionary(state->run.parameters, size);
    return 0;
}

int BoltProtocolV1_set_cypher_parameter_key(struct BoltConnection * connection, int32_t index, const char * key,
                                            size_t key_size)
{
    struct BoltProtocolV1State * state = BoltProtocolV1_state(connection);
    return BoltDictionary_set_key(state->run.parameters, index, key, key_size);
}

struct BoltValue * BoltProtocolV1_cypher_parameter_value(struct BoltConnection * connection, int32_t index)
{
    struct BoltProtocolV1State * state = BoltProtocolV1_state(connection);
    return BoltDictionary_value(state->run.parameters, index);
}

int BoltProtocolV1_load_bookmark(struct BoltConnection * connection, const char * bookmark)
{
    if (bookmark == NULL)
    {
        return 0;
    }
    struct BoltProtocolV1State * state = BoltProtocolV1_state(connection);
    struct BoltValue * bookmarks;
    if (state->begin.parameters->size == 0)
    {
        BoltValue_to_Dictionary(state->begin.parameters, 1);
        BoltDictionary_set_key(state->begin.parameters, 0, "bookmarks", 9);
        bookmarks = BoltDictionary_value(state->begin.parameters, 0);
        BoltValue_to_List(bookmarks, 0);
    }
    else
    {
        bookmarks = BoltDictionary_value(state->begin.parameters, 0);
    }
    int32_t n_bookmarks = bookmarks->size;
    BoltList_resize(bookmarks, n_bookmarks + 1);
    size_t bookmark_size = strlen(bookmark);
    if (bookmark_size > INT32_MAX)
    {
        return -1;
    }
    BoltValue_to_String(BoltList_value(bookmarks, n_bookmarks), bookmark, (int32_t)(bookmark_size));
    return 1;
}

int BoltProtocolV1_load_begin_request(struct BoltConnection * connection)
{
    struct BoltProtocolV1State* state = BoltProtocolV1_state(connection);
    BoltProtocolV1_load_message(connection, state->begin.request);
    BoltValue_to_Dictionary(state->begin.parameters, 0);
    BoltProtocolV1_load_message(connection, state->discard_request);
    return 0;
}

int BoltProtocolV1_load_commit_request(struct BoltConnection * connection)
{
    struct BoltProtocolV1State * state = BoltProtocolV1_state(connection);
    BoltProtocolV1_load_message(connection, state->commit.request);
    BoltProtocolV1_load_message(connection, state->discard_request);
    return 0;
}

int BoltProtocolV1_load_rollback_request(struct BoltConnection * connection)
{
    struct BoltProtocolV1State * state = BoltProtocolV1_state(connection);
    BoltProtocolV1_load_message(connection, state->rollback.request);
    BoltProtocolV1_load_message(connection, state->discard_request);
    return 0;
}

int BoltProtocolV1_load_run_request(struct BoltConnection * connection)
{
    struct BoltProtocolV1State * state = BoltProtocolV1_state(connection);
    BoltProtocolV1_load_message(connection, state->run.request);
    return 0;
}

int BoltProtocolV1_load_pull_request(struct BoltConnection * connection, int32_t n)
{
    if (n >= 0)
    {
        return -1;
    }
    else
    {
        struct BoltProtocolV1State * state = BoltProtocolV1_state(connection);
        BoltProtocolV1_load_message(connection, state->pull_request);
        return 0;
    }
}

int32_t BoltProtocolV1_n_fields(struct BoltConnection * connection)
{
    struct BoltProtocolV1State * state = BoltProtocolV1_state(connection);
    switch (BoltValue_type(state->fields))
    {
        case BOLT_STRING_ARRAY:
            return state->fields->size;
        default:
            return -1;
    }
}

const char * BoltProtocolV1_field_name(struct BoltConnection * connection, int32_t index)
{
    struct BoltProtocolV1State * state = BoltProtocolV1_state(connection);
    switch (BoltValue_type(state->fields))
    {
        case BOLT_STRING_ARRAY:
            if (index >= 0 && index < state->fields->size)
            {
                return BoltStringArray_get(state->fields, index);
            }
            else
            {
                return NULL;
            }
        default:
            return NULL;
    }
}

int32_t BoltProtocolV1_field_name_size(struct BoltConnection * connection, int32_t index)
{
    struct BoltProtocolV1State * state = BoltProtocolV1_state(connection);
    switch (BoltValue_type(state->fields))
    {
        case BOLT_STRING_ARRAY:
            if (index >= 0 && index < state->fields->size)
            {
                return BoltStringArray_get_size(state->fields, index);
            }
            else
            {
                return -1;
            }
        default:
            return -1;
    }
}

int BoltProtocolV1_dump(struct BoltValue * value, struct BoltBuffer * buffer)
{
    return load(buffer, value);
}
