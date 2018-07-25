/*
 * Copyright (c) 2002-2018 "Neo Technology,"
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

#include "v1.h"

#define POINT_2D            'X'
#define POINT_3D            'Y'
#define LOCAL_DATE          'D'
#define LOCAL_TIME          't'
#define LOCAL_DATE_TIME     'd'
#define OFFSET_TIME         'T'
#define OFFSET_DATE_TIME    'F'
#define ZONED_DATE_TIME     'f'
#define DURATION            'E'

int BoltProtocolV2_check_readable_struct_signature(int16_t signature)
{
    if (BoltProtocolV1_check_readable_struct_signature(signature)) {
        return 1;
    }

    switch (signature) {
    case POINT_2D:
    case POINT_3D:
    case LOCAL_DATE:
    case LOCAL_DATE_TIME:
    case LOCAL_TIME:
    case OFFSET_TIME:
    case OFFSET_DATE_TIME:
    case ZONED_DATE_TIME:
    case DURATION:
        return 1;
    }

    return 0;
}

int BoltProtocolV2_check_writable_struct_signature(int16_t signature)
{
    if (BoltProtocolV1_check_writable_struct_signature(signature)) {
        return 1;
    }

    switch (signature) {
    case POINT_2D:
    case POINT_3D:
    case LOCAL_DATE:
    case LOCAL_DATE_TIME:
    case LOCAL_TIME:
    case OFFSET_TIME:
    case OFFSET_DATE_TIME:
    case ZONED_DATE_TIME:
    case DURATION:
        return 1;
    }

    return 0;
}

struct BoltProtocolV1State* BoltProtocolV2_create_state()
{
    struct BoltProtocolV1State* v1_state = BoltProtocolV1_create_state();

    v1_state->check_writable_struct = &BoltProtocolV2_check_writable_struct_signature;
    v1_state->check_readable_struct = &BoltProtocolV2_check_readable_struct_signature;

    return v1_state;
}