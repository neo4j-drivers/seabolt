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

#include "catch.hpp"
#include "integration.hpp"

SCENARIO("BoltConnection")
{
    GIVEN("a new constructed BoltConnection") {
        BoltConnection* connection = BoltConnection_create();

        THEN("access mode should be set to WRITE") {
            REQUIRE(connection->access_mode==BOLT_ACCESS_MODE_WRITE);
        }

        THEN("status is not null") {
            REQUIRE(connection->status!=NULL);
        }

        THEN("status->state should be DISCONNECTED") {
            REQUIRE(connection->status->state==BOLT_CONNECTION_STATE_DISCONNECTED);
        }

        THEN("status->error should be SUCCESS") {
            REQUIRE(connection->status->error==BOLT_SUCCESS);
        }

        THEN("status->error_ctx should not be NULL") {
            REQUIRE(connection->status->error_ctx!=NULL);
        }

        THEN("metrics is not null") {
            REQUIRE(connection->metrics!=NULL);
        }

        BoltConnection_destroy(connection);
    }
}