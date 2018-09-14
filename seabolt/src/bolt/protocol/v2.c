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

const char* BoltProtocolV2_structure_name(int16_t code)
{
    switch (code) {
    case POINT_2D:
        return "Point2D";
    case POINT_3D:
        return "Point3D";
    case LOCAL_DATE:
        return "LocalDate";
    case LOCAL_TIME:
        return "LocalTime";
    case LOCAL_DATE_TIME:
        return "LocalDateTime";
    case OFFSET_TIME:
        return "OffsetTime";
    case OFFSET_DATE_TIME:
        return "OffsetDateTime";
    case ZONED_DATE_TIME:
        return "ZonedDateTime";
    case DURATION:
        return "Duration";
    default:
        return BoltProtocolV1_structure_name(code);
    }
}

struct BoltProtocol* BoltProtocolV2_create_protocol()
{
    struct BoltProtocol* v1_protocol = BoltProtocolV1_create_protocol();

    // Overrides for new structure types
    v1_protocol->structure_name = &BoltProtocolV2_structure_name;
    v1_protocol->check_writable_struct = &BoltProtocolV2_check_writable_struct_signature;
    v1_protocol->check_readable_struct = &BoltProtocolV2_check_readable_struct_signature;

    return v1_protocol;
}

void BoltProtocolV2_destroy_protocol(struct BoltProtocol* protocol)
{
    BoltProtocolV1_destroy_protocol(protocol);
}