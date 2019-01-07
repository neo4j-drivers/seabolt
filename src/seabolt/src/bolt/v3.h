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
#ifndef SEABOLT_ALL_V3_H
#define SEABOLT_ALL_V3_H

#include "connection.h"

#define BOLT_V3_HELLO 0x01
#define BOLT_V3_GOODBYE 0x02
#define BOLT_V3_RUN 0x10
#define BOLT_V3_BEGIN 0x11
#define BOLT_V3_COMMIT 0x12
#define BOLT_V3_ROLLBACK 0x13
#define BOLT_V3_DISCARD_ALL 0x2F
#define BOLT_V3_PULL_ALL    0x3F
#define BOLT_V3_RESET       0x0F

#define BOLT_V3_SUCCESS 0x70
#define BOLT_V3_RECORD  0x71
#define BOLT_V3_IGNORED 0x7E
#define BOLT_V3_FAILURE 0x7F

#define BOLT_V3_NODE    'N'
#define BOLT_V3_RELATIONSHIP 'R'
#define BOLT_V3_UNBOUND_RELATIONSHIP 'r'
#define BOLT_V3_PATH 'P'
#define BOLT_V3_POINT_2D            'X'
#define BOLT_V3_POINT_3D            'Y'
#define BOLT_V3_LOCAL_DATE          'D'
#define BOLT_V3_LOCAL_TIME          't'
#define BOLT_V3_LOCAL_DATE_TIME     'd'
#define BOLT_V3_OFFSET_TIME         'T'
#define BOLT_V3_OFFSET_DATE_TIME    'F'
#define BOLT_V3_ZONED_DATE_TIME     'f'
#define BOLT_V3_DURATION            'E'


void BoltProtocolV3_extract_metadata(struct BoltConnection* connection, struct BoltValue* metadata);

struct BoltProtocol* BoltProtocolV3_create_protocol();

void BoltProtocolV3_destroy_protocol(struct BoltProtocol* protocol);

#endif //SEABOLT_ALL_V3_H
