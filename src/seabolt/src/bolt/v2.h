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

#ifndef SEABOLT_ALL_V2_H
#define SEABOLT_ALL_V2_H

#define BOLT_V2_POINT_2D            'X'
#define BOLT_V2_POINT_3D            'Y'
#define BOLT_V2_LOCAL_DATE          'D'
#define BOLT_V2_LOCAL_TIME          't'
#define BOLT_V2_LOCAL_DATE_TIME     'd'
#define BOLT_V2_OFFSET_TIME         'T'
#define BOLT_V2_OFFSET_DATE_TIME    'F'
#define BOLT_V2_ZONED_DATE_TIME     'f'
#define BOLT_V2_DURATION            'E'

struct BoltProtocol* BoltProtocolV2_create_protocol();

void BoltProtocolV2_destroy_protocol(struct BoltProtocol* protocol);

#endif //SEABOLT_ALL_V2_H
