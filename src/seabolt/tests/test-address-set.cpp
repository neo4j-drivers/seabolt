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

SCENARIO("BoltAddressSet")
{
    struct BoltAddress* localhost7687 = BoltAddress_create("localhost", "7687");
    struct BoltAddress* localhost7688 = BoltAddress_create("localhost", "7688");
    struct BoltAddress* localhost7689 = BoltAddress_create("localhost", "7689");
    struct BoltAddress* localhost7690 = BoltAddress_create("localhost", "7690");

    WHEN("constructed") {
        struct BoltAddressSet* set = BoltAddressSet_create();

        THEN("it should have size = 0") {
            REQUIRE(set->size==0);
        }

        THEN("it should have elements = NULL") {
            REQUIRE(set->elements==nullptr);
        }

        THEN("it should report size = 0") {
            REQUIRE(BoltAddressSet_size(set)==0);
        }
    }

    GIVEN("a newly constructed BoltAddressSet") {
        struct BoltAddressSet* set = BoltAddressSet_create();

        WHEN("BoltAddress[localhost,7687] is added") {
            BoltAddressSet_add(set, localhost7687);

            THEN("it should have size = 1") {
                REQUIRE(BoltAddressSet_size(set)==1);
            }

            THEN("it should report index of = 0") {
                REQUIRE(BoltAddressSet_index_of(set, localhost7687)==0);
            }
        }

        WHEN("BoltAddress[localhost,7687] is added again") {
            BoltAddressSet_add(set, localhost7687);

            THEN("it should have size = 1") {
                REQUIRE(BoltAddressSet_size(set)==1);
            }

            THEN("it should report index of = 0") {
                REQUIRE(BoltAddressSet_index_of(set, localhost7687)==0);
            }
        }

        WHEN("BoltAddress[localhost,7687] and BoltAddress[localhost,7688] is added") {
            BoltAddressSet_add(set, localhost7687);
            BoltAddressSet_add(set, localhost7688);

            THEN("it should have size = 2") {
                REQUIRE(BoltAddressSet_size(set)==2);
            }

            THEN("it should report index of BoltAddress[localhost,7687] = 0") {
                REQUIRE(BoltAddressSet_index_of(set, localhost7687)==0);
            }

            THEN("it should report index of BoltAddress[localhost,7688] = 1") {
                REQUIRE(BoltAddressSet_index_of(set, localhost7688)==1);
            }

            THEN("it should report index of BoltAddress[localhost,7689] = -1") {
                REQUIRE(BoltAddressSet_index_of(set, localhost7689)==-1);
            }
        }

        BoltAddressSet_destroy(set);
    }

    GIVEN("A BoltAddressSet with 3 addresses") {
        struct BoltAddressSet* set = BoltAddressSet_create();

        BoltAddressSet_add(set, localhost7687);
        BoltAddressSet_add(set, localhost7688);
        BoltAddressSet_add(set, localhost7689);

        WHEN("index of BoltAddress[localhost,7689] is queried") {
            THEN("it should report = 2") {
                REQUIRE(BoltAddressSet_index_of(set, localhost7689)==2);
            }
        }

        WHEN("index of BoltAddress[localhost,7690] is queried") {
            THEN("it should report = -1") {
                REQUIRE(BoltAddressSet_index_of(set, localhost7690)==-1);
            }
        }

        WHEN("BoltAddress[localhost,7689] is added again") {
            THEN("it should return -1") {
                REQUIRE(BoltAddressSet_add(set, localhost7689)==-1);
            }
        }

        WHEN("BoltAddress[localhost,7689] is removed") {
            REQUIRE(BoltAddressSet_remove(set, localhost7689)==2);

            THEN("it should have size = 2") {
                REQUIRE(BoltAddressSet_size(set)==2);
            }

            AND_WHEN("BoltAddress[localhost,7689] is removed again") {
                THEN("it should return -1") {
                    REQUIRE(BoltAddressSet_remove(set, localhost7689)==-1);
                }

                THEN("it should have size = 2") {
                    REQUIRE(BoltAddressSet_size(set)==2);
                }
            }
        }

        BoltAddressSet_destroy(set);
    }

    GIVEN("Two different BoltAddressSets") {
        struct BoltAddressSet* set1 = BoltAddressSet_create();
        REQUIRE(BoltAddressSet_add(set1, localhost7687)==0);
        REQUIRE(BoltAddressSet_add(set1, localhost7688)==1);

        struct BoltAddressSet* set2 = BoltAddressSet_create();
        REQUIRE(BoltAddressSet_add(set2, localhost7689)==0);

        WHEN("set1 is updated from set2") {
            BoltAddressSet_replace(set1, set2);

            THEN("set1 should have size = 1") {
                REQUIRE(BoltAddressSet_size(set1)==1);
            }

            THEN("set1 should contain BoltAddress[localhost,7689]") {
                REQUIRE(BoltAddressSet_index_of(set1, localhost7689)==0);
            }

            THEN("set1 should not contain BoltAddress[localhost,7687]") {
                REQUIRE(BoltAddressSet_index_of(set1, localhost7687)==-1);
            }

            THEN("set1 should not contain BoltAddress[localhost,7688]") {
                REQUIRE(BoltAddressSet_index_of(set1, localhost7688)==-1);
            }
        }
    }

    GIVEN("Two different BoltAddressSets") {
        struct BoltAddressSet* set1 = BoltAddressSet_create();
        REQUIRE(BoltAddressSet_add(set1, localhost7689)==0);

        struct BoltAddressSet* set2 = BoltAddressSet_create();
        REQUIRE(BoltAddressSet_add(set2, localhost7687)==0);
        REQUIRE(BoltAddressSet_add(set2, localhost7688)==1);
        REQUIRE(BoltAddressSet_add(set2, localhost7689)==2);

        WHEN("set2 is added to set1") {
            BoltAddressSet_add_all(set1, set2);

            THEN("set1 should have size = 1") {
                REQUIRE(BoltAddressSet_size(set1)==3);
            }

            THEN("set1 should contain BoltAddress[localhost,7689]") {
                REQUIRE(BoltAddressSet_index_of(set1, localhost7689)==0);
            }

            THEN("set1 should not contain BoltAddress[localhost,7687]") {
                REQUIRE(BoltAddressSet_index_of(set1, localhost7687)>=0);
            }

            THEN("set1 should not contain BoltAddress[localhost,7688]") {
                REQUIRE(BoltAddressSet_index_of(set1, localhost7688)>=0);
            }
        }
    }

    BoltAddress_destroy(localhost7687);
    BoltAddress_destroy(localhost7688);
    BoltAddress_destroy(localhost7689);
    BoltAddress_destroy(localhost7690);
}