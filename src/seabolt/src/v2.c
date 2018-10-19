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
#include "v2.h"

int BoltProtocolV2_check_readable_struct_signature(int16_t signature)
{
    if (BoltProtocolV1_check_readable_struct_signature(signature)) {
        return 1;
    }

    switch (signature) {
    case BOLT_V2_POINT_2D:
    case BOLT_V2_POINT_3D:
    case BOLT_V2_LOCAL_DATE:
    case BOLT_V2_LOCAL_DATE_TIME:
    case BOLT_V2_LOCAL_TIME:
    case BOLT_V2_OFFSET_TIME:
    case BOLT_V2_OFFSET_DATE_TIME:
    case BOLT_V2_ZONED_DATE_TIME:
    case BOLT_V2_DURATION:
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
    case BOLT_V2_POINT_2D:
    case BOLT_V2_POINT_3D:
    case BOLT_V2_LOCAL_DATE:
    case BOLT_V2_LOCAL_DATE_TIME:
    case BOLT_V2_LOCAL_TIME:
    case BOLT_V2_OFFSET_TIME:
    case BOLT_V2_OFFSET_DATE_TIME:
    case BOLT_V2_ZONED_DATE_TIME:
    case BOLT_V2_DURATION:
        return 1;
    }

    return 0;
}

const char* BoltProtocolV2_structure_name(int16_t code)
{
    switch (code) {
    case BOLT_V2_POINT_2D:
        return "Point2D";
    case BOLT_V2_POINT_3D:
        return "Point3D";
    case BOLT_V2_LOCAL_DATE:
        return "LocalDate";
    case BOLT_V2_LOCAL_TIME:
        return "LocalTime";
    case BOLT_V2_LOCAL_DATE_TIME:
        return "LocalDateTime";
    case BOLT_V2_OFFSET_TIME:
        return "OffsetTime";
    case BOLT_V2_OFFSET_DATE_TIME:
        return "OffsetDateTime";
    case BOLT_V2_ZONED_DATE_TIME:
        return "ZonedDateTime";
    case BOLT_V2_DURATION:
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

