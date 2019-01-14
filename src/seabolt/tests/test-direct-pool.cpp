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

#include "integration.hpp"
#include "catch.hpp"

TEST_CASE("Direct Pool", "[unit]")
{
    BoltAddress* address = BoltAddress_create("localhost", "8888");
    BoltValue* auth_token = BoltAuth_basic("user", "password", NULL);
    BoltConfig* config = BoltConfig_create();

    SECTION("Pool is Full") {
        SECTION("max_connection_acquisition_time>0") {
            BoltDirectPool* pool = nullptr;

            SECTION("should fail with BOLT_POOL_ACQUISITION_TIMED_OUT") {
                BoltConfig_set_max_pool_size(config, 10);
                BoltConfig_set_max_connection_acquisition_time(config, 1000);

                pool = BoltDirectPool_create(address, auth_token, config);
                // fake the pool to believe all of its connections are in use
                for (int i = 0; i<pool->size; i++) {
                    pool->connections[i]->agent = "USED";
                }

                BoltStatus* status = BoltStatus_create();
                BoltConnection* connection = BoltDirectPool_acquire(pool, status);

                REQUIRE(connection==nullptr);
                REQUIRE(status->error==BOLT_POOL_ACQUISITION_TIMED_OUT);
            }

            BoltDirectPool_destroy(pool);
        }

        SECTION("max_connection_acquisition_time=0") {
            BoltDirectPool* pool = nullptr;

            SECTION("should fail with BOLT_POOL_FULL") {
                BoltConfig_set_max_pool_size(config, 10);
                BoltConfig_set_max_connection_acquisition_time(config, 0);

                pool = BoltDirectPool_create(address, auth_token, config);
                // fake the pool to believe all of its connections are in use
                for (int i = 0; i<pool->size; i++) {
                    pool->connections[i]->agent = "USED";
                }

                BoltStatus* status = BoltStatus_create();
                BoltConnection* connection = BoltDirectPool_acquire(pool, status);

                REQUIRE(connection==nullptr);
                REQUIRE(status->error==BOLT_POOL_FULL);
            }

            BoltDirectPool_destroy(pool);
        }
    }

    BoltConfig_destroy(config);
    BoltValue_destroy(auth_token);
    BoltAddress_destroy(address);
}
