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

#ifndef SEABOLT_DUMP
#define SEABOLT_DUMP

#include <printf.h>
#include <stdlib.h>
#include <memory.h>

#include "../seabolt/values.h"

const char HEX_DIGITS[] = {'0', '1', '2', '3', '4', '5', '6', '7',
                           '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

char hex_hi(const char* mem, unsigned long offset)
{
    return HEX_DIGITS[(mem[offset] >> 4) & 0x0F];
}

char hex_lo(const char* mem, unsigned long offset)
{
    return HEX_DIGITS[mem[offset] & 0x0F];
}

int bolt_dump(struct BoltValue* x)
{
    printf("%d : ", (int)(x->channel));
    switch (x->type)
    {
        case BOLT_NULL:
            printf("~");
            return EXIT_SUCCESS;
        case BOLT_BIT:
            printf("b(%d)", bolt_get_bit(x));
            return EXIT_SUCCESS;
        case BOLT_BIT_ARRAY:
            printf("b[");
            for (int i = 0; i < x->data_items; i++)
            {
                printf("%d", bolt_get_bit_array_at(x, i));
            }
            printf("]");
            return EXIT_SUCCESS;
        case BOLT_BYTE:
        {
            char byte = bolt_get_byte(x);
            printf("b8(#%c%c)", hex_hi(&byte, 0), hex_lo(&byte, 0));
            return EXIT_SUCCESS;
        }
        case BOLT_BYTE_ARRAY:
        {
            printf("b8[#");
            for (int i = 0; i < x->data_items; i++)
            {
                char value = bolt_get_byte_array_at(x, i);
                printf("%c%c", hex_hi(&value, 0), hex_lo(&value, 0));
            }
            printf("]");
            return EXIT_SUCCESS;
        }
        case BOLT_UTF8:
            printf("u8(\"");
            for (int i = 0; i < x->data_items; i++)
            {
                printf("%c", x->data.as_char[i]);
            }
            printf("\")");
            break;
        case BOLT_UTF8_ARRAY:
            printf("u8[");
            char* data = x->data.as_char;
            int size;
            for (unsigned long i = 0; i < x->data_items; i++)
            {
                if (i > 0) { printf(", "); }
                memcpy(&size, data, SIZE_OF_SIZE);
                data += SIZE_OF_SIZE;
                printf("\"");
                for (unsigned long j = 0; j < size; j++)
                {
                    printf("%c", data[j]);
                }
                printf("\"");
                data += size;
            }
            printf("]");
            break;
        case BOLT_NUM8:
            printf("n8(%d)", bolt_get_num8(x));
            return EXIT_SUCCESS;
        case BOLT_NUM8_ARRAY:
            printf("n8[");
            for (int i = 0; i < x->data_items; i++)
            {
                printf(i == 0 ? "%d" : ", %d", bolt_get_num8_array_at(x, i));
            }
            printf("]");
            return EXIT_SUCCESS;
        case BOLT_NUM16:
            printf("n16(%d)", bolt_get_num16(x));
            break;
        case BOLT_NUM16_ARRAY:
            printf("n16[");
            for (int i = 0; i < x->data_items; i++)
            {
                printf(i == 0 ? "%d" : ", %d", bolt_get_num16_array_at(x, i));
            }
            printf("]");
            break;
        case BOLT_NUM32:
            printf("n32(%u)", bolt_get_num32(x));
            break;
        case BOLT_NUM32_ARRAY:
            printf("n32[");
            for (int i = 0; i < x->data_items; i++)
            {
                printf(i == 0 ? "%u" : ", %u", bolt_get_num32_array_at(x, i));
            }
            printf("]");
            break;
        case BOLT_NUM64:
            printf("n64(%lu)", bolt_get_num64(x));
            break;
        case BOLT_NUM64_ARRAY:
            printf("n64[");
            for (int i = 0; i < x->data_items; i++)
            {
                printf(i == 0 ? "%lu" : ", %lu", bolt_get_num64_array_at(x, i));
            }
            printf("]");
            break;
        case BOLT_INT8:
            printf("i8(%d)", bolt_get_int8(x));
            break;
        case BOLT_INT8_ARRAY:
            printf("i8[");
            for (int i = 0; i < x->data_items; i++)
            {
                printf(i == 0 ? "%d" : ", %d", bolt_get_int8_array_at(x, i));
            }
            printf("]");
            break;
        case BOLT_INT16:
            printf("i16(%d)", bolt_get_int16(x));
            break;
        case BOLT_INT16_ARRAY:
            printf("i16[");
            for (int i = 0; i < x->data_items; i++)
            {
                printf(i == 0 ? "%d" : ", %d", bolt_get_int16_array_at(x, i));
            }
            printf("]");
            break;
        case BOLT_INT32:
            printf("i32(%d)", bolt_get_int32(x));
            break;
        case BOLT_INT32_ARRAY:
            printf("i32[");
            for (int i = 0; i < x->data_items; i++)
            {
                printf(i == 0 ? "%d" : ", %d", bolt_get_int32_array_at(x, i));
            }
            printf("]");
            break;
        case BOLT_INT64:
            printf("i64(%ld)", bolt_get_int64(x));
            break;
        case BOLT_INT64_ARRAY:
            printf("i64[");
            for (int i = 0; i < x->data_items; i++)
            {
                printf(i == 0 ? "%ld" : ", %ld", bolt_get_int64_array_at(x, i));
            }
            printf("]");
            break;
        case BOLT_FLOAT32:
            printf("f32(%f)", bolt_get_float32(x));
            break;
        case BOLT_FLOAT32_ARRAY:
            printf("f32[");
            for (int i = 0; i < x->data_items; i++)
            {
                printf(i == 0 ? "%f" : ", %f", bolt_get_float32_array_at(x, i));
            }
            printf("]");
            break;
        default:
            printf("?");
    }
    return EXIT_SUCCESS;
}

void bolt_dump_ln(struct BoltValue* value)
{
    bolt_dump(value);
    printf("\n");
}

#include <stdio.h>
#include "../seabolt/values.h"

#endif // SEABOLT_DUMP
