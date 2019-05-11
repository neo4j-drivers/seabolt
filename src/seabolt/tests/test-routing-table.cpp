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

TEST_CASE("Routing Table", "[unit]")
{
    BoltAddress* server1 = BoltAddress_create("localhost", "8080");
    BoltAddress* server2 = BoltAddress_create("localhost", "8081");
    BoltAddress* server3 = BoltAddress_create("localhost", "8082");
    BoltAddress* server4 = BoltAddress_create("localhost", "8083");
    BoltAddress* server5 = BoltAddress_create("localhost", "8084");
    BoltAddress* server6 = BoltAddress_create("localhost", "8085");
    BoltAddress* server7 = BoltAddress_create("localhost", "8086");

    SECTION("RoutingTable_create") {
        volatile RoutingTable* table = RoutingTable_create();

        REQUIRE(table->expires==0);
        REQUIRE(table->last_updated==0);
        REQUIRE(table->readers!=nullptr);
        REQUIRE(BoltAddressSet_size(table->readers)==0);
        REQUIRE(table->writers!=nullptr);
        REQUIRE(BoltAddressSet_size(table->writers)==0);
        REQUIRE(table->routers!=nullptr);
        REQUIRE(BoltAddressSet_size(table->routers)==0);

        RoutingTable_destroy(table);
    }

    SECTION("RoutingTable_update") {
        volatile RoutingTable* table = RoutingTable_create();

        SECTION("should return error if response is not Dictionary but is ") {
            BoltValue* response = BoltValue_create();

            SECTION("null") {
                BoltValue_format_as_Null(response);

                REQUIRE(RoutingTable_update(table, response)==BOLT_ROUTING_UNEXPECTED_DISCOVERY_RESPONSE);
            }
            SECTION("boolean") {
                BoltValue_format_as_Boolean(response, '1');

                REQUIRE(RoutingTable_update(table, response)==BOLT_ROUTING_UNEXPECTED_DISCOVERY_RESPONSE);
            }
            SECTION("integer") {
                BoltValue_format_as_Integer(response, 0);

                REQUIRE(RoutingTable_update(table, response)==BOLT_ROUTING_UNEXPECTED_DISCOVERY_RESPONSE);
            }
            SECTION("float") {
                BoltValue_format_as_Float(response, 0.1);

                REQUIRE(RoutingTable_update(table, response)==BOLT_ROUTING_UNEXPECTED_DISCOVERY_RESPONSE);
            }
            SECTION("string") {
                BoltValue_format_as_String(response, "test string", 11);

                REQUIRE(RoutingTable_update(table, response)==BOLT_ROUTING_UNEXPECTED_DISCOVERY_RESPONSE);
            }
            SECTION("list") {
                BoltValue_format_as_List(response, 0);

                REQUIRE(RoutingTable_update(table, response)==BOLT_ROUTING_UNEXPECTED_DISCOVERY_RESPONSE);
            }
            SECTION("bytes") {
                BoltValue_format_as_Bytes(response, (char*) "0123", 3);

                REQUIRE(RoutingTable_update(table, response)==BOLT_ROUTING_UNEXPECTED_DISCOVERY_RESPONSE);
            }
            SECTION("structure") {
                BoltValue_format_as_Structure(response, 12, 0);

                REQUIRE(RoutingTable_update(table, response)==BOLT_ROUTING_UNEXPECTED_DISCOVERY_RESPONSE);
            }

            BoltValue_destroy(response);
        }

        SECTION("should return error if response is invalid") {
            BoltValue* response = BoltValue_create();
            BoltValue_format_as_Dictionary(response, 0);

            SECTION("because TTL is not present") {
                REQUIRE(RoutingTable_update(table, response)==BOLT_ROUTING_UNEXPECTED_DISCOVERY_RESPONSE);
            }

            BoltValue_format_as_Dictionary(response, 1);
            BoltDictionary_set_key(response, 0, "ttl", 3);
            BoltValue* ttl_value = BoltDictionary_value(response, 0);

            SECTION("because TTL is not INTEGER") {
                BoltValue_format_as_Boolean(ttl_value, 0);

                REQUIRE(RoutingTable_update(table, response)==BOLT_ROUTING_UNEXPECTED_DISCOVERY_RESPONSE);
            }

            BoltValue_format_as_Integer(ttl_value, 500);

            SECTION("because SERVERS is not present") {
                REQUIRE(RoutingTable_update(table, response)==BOLT_ROUTING_UNEXPECTED_DISCOVERY_RESPONSE);
            }

            BoltValue_format_as_Dictionary(response, 2);
            BoltDictionary_set_key(response, 1, "servers", 7);
            BoltValue* servers_value = BoltDictionary_value(response, 1);

            SECTION("because SERVERS is not LIST") {
                BoltValue_format_as_String(servers_value, "test", 4);

                REQUIRE(RoutingTable_update(table, response)==BOLT_ROUTING_UNEXPECTED_DISCOVERY_RESPONSE);
            }

            BoltValue_format_as_List(servers_value, 1);
            BoltValue* server_value = BoltList_value(servers_value, 0);

            SECTION("because SERVERS[0] is not DICTIONARY") {
                BoltValue_format_as_Float(server_value, 5.5);

                REQUIRE(RoutingTable_update(table, response)==BOLT_ROUTING_UNEXPECTED_DISCOVERY_RESPONSE);
            }

            BoltValue_format_as_Dictionary(server_value, 0);

            SECTION("because SERVERS[0].ROLE is not present") {
                REQUIRE(RoutingTable_update(table, response)==BOLT_ROUTING_UNEXPECTED_DISCOVERY_RESPONSE);
            }

            BoltValue_format_as_Dictionary(server_value, 1);
            BoltDictionary_set_key(server_value, 0, "role", 7);
            BoltValue* role_value = BoltDictionary_value(server_value, 0);

            SECTION("because SERVERS[0].ROLE is not STRING") {
                BoltValue_format_as_Integer(role_value, 0);

                REQUIRE(RoutingTable_update(table, response)==BOLT_ROUTING_UNEXPECTED_DISCOVERY_RESPONSE);
            }

            BoltValue_format_as_String(role_value, "", 0);

            SECTION("because SERVERS[0].ADDRESSES is not present") {
                REQUIRE(RoutingTable_update(table, response)==BOLT_ROUTING_UNEXPECTED_DISCOVERY_RESPONSE);
            }

            BoltValue_format_as_Dictionary(server_value, 2);
            BoltDictionary_set_key(server_value, 1, "addresses", 9);
            role_value = BoltDictionary_value(server_value, 0);
            BoltValue* addresses_value = BoltDictionary_value(server_value, 1);

            SECTION("because SERVERS[0].ADDRESSES is not LIST") {
                BoltValue_format_as_Dictionary(addresses_value, 0);

                REQUIRE(RoutingTable_update(table, response)==BOLT_ROUTING_UNEXPECTED_DISCOVERY_RESPONSE);
            }

            BoltValue_format_as_List(addresses_value, 1);
            BoltValue* address_value = BoltList_value(addresses_value, 0);

            SECTION("because SERVERS[0].ADDRESSES[0] is not STRING") {
                BoltValue_format_as_Boolean(address_value, 0);

                REQUIRE(RoutingTable_update(table, response)==BOLT_ROUTING_UNEXPECTED_DISCOVERY_RESPONSE);
            }

            BoltValue_format_as_String(address_value, "localhost.local.domain:7687", 27);

            SECTION("because SERVERS[0].ROLE is not one of the expected values") {
                BoltValue_format_as_String(role_value, "other_role", 10);

                REQUIRE(RoutingTable_update(table, response)==BOLT_ROUTING_UNEXPECTED_DISCOVERY_RESPONSE);
            }

            BoltValue_format_as_String(role_value, "router", 6);

            SECTION("because no readers are present") {
                REQUIRE(RoutingTable_update(table, response)==BOLT_ROUTING_UNEXPECTED_DISCOVERY_RESPONSE);
            }

            BoltValue_destroy(response);
        }

        SECTION("should update successfully") {
            SECTION("when one router and one reader is present") {
                BoltValue* response = BoltValue_create();
                BoltValue_format_as_Dictionary(response, 2);

                BoltDictionary_set_key(response, 0, "ttl", 3);
                BoltValue* ttl_value = BoltDictionary_value(response, 0);
                BoltValue_format_as_Integer(ttl_value, 500);

                BoltDictionary_set_key(response, 1, "servers", 7);
                BoltValue* servers_value = BoltDictionary_value(response, 1);
                BoltValue_format_as_List(servers_value, 2);

                BoltValue* server1_value = BoltList_value(servers_value, 0);
                BoltValue_format_as_Dictionary(server1_value, 2);
                BoltDictionary_set_key(server1_value, 0, "role", 4);
                BoltValue* role1_value = BoltDictionary_value(server1_value, 0);
                BoltValue_format_as_String(role1_value, "ROUTE", 5);
                BoltDictionary_set_key(server1_value, 1, "addresses", 9);
                BoltValue* addresses1_value = BoltDictionary_value(server1_value, 1);
                BoltValue_format_as_List(addresses1_value, 1);
                BoltValue* address1_value = BoltList_value(addresses1_value, 0);
                BoltValue_format_as_String(address1_value, "localhost:8080", 27);

                BoltValue* server2_value = BoltList_value(servers_value, 1);
                BoltValue_format_as_Dictionary(server2_value, 2);
                BoltDictionary_set_key(server2_value, 0, "role", 4);
                BoltValue* role2_value = BoltDictionary_value(server2_value, 0);
                BoltValue_format_as_String(role2_value, "READ", 4);
                BoltDictionary_set_key(server2_value, 1, "addresses", 9);
                BoltValue* addresses2_value = BoltDictionary_value(server2_value, 1);
                BoltValue_format_as_List(addresses2_value, 1);
                BoltValue* address2_value = BoltList_value(addresses2_value, 0);
                BoltValue_format_as_String(address2_value, "localhost:8081", 27);

                REQUIRE(RoutingTable_update(table, response)==BOLT_SUCCESS);

                REQUIRE(BoltAddressSet_size(table->routers)==1);
                REQUIRE(BoltAddressSet_index_of(table->routers, server1)==0);
                REQUIRE(BoltAddressSet_size(table->readers)==1);
                REQUIRE(BoltAddressSet_index_of(table->readers, server2)==0);
                REQUIRE(BoltAddressSet_size(table->writers)==0);
                REQUIRE(table->expires-table->last_updated==500000);
            }
        }

        RoutingTable_destroy(table);
    }

    SECTION("RoutingTable_is_expired") {
        volatile RoutingTable* table = RoutingTable_create();

        SECTION("should return true on construction") {
            SECTION("READ") {
                REQUIRE(RoutingTable_is_expired(table, BOLT_ACCESS_MODE_READ)==1);
            }

            SECTION("WRITE") {
                REQUIRE(RoutingTable_is_expired(table, BOLT_ACCESS_MODE_WRITE)==1);
            }
        }

        SECTION("should return true when ttl expired") {
            uint64_t time = BoltTime_get_time_ms();
            table->expires = time+2000;
            table->last_updated = time;

            BoltSync_sleep(3000);

            SECTION("READ") {
                REQUIRE(RoutingTable_is_expired(table, BOLT_ACCESS_MODE_READ)==1);
            }

            SECTION("WRITE") {
                REQUIRE(RoutingTable_is_expired(table, BOLT_ACCESS_MODE_WRITE)==1);
            }
        }

        SECTION("should return false when ttl is not expired") {
            uint64_t time = BoltTime_get_time_ms();
            table->expires = time+5000;
            table->last_updated = time;

            BoltAddressSet_add(table->routers, server1);

            SECTION("READ") {
                BoltAddressSet_add(table->readers, server2);
                REQUIRE(RoutingTable_is_expired(table, BOLT_ACCESS_MODE_READ)==0);
            }

            SECTION("WRITE") {
                BoltAddressSet_add(table->writers, server3);
                REQUIRE(RoutingTable_is_expired(table, BOLT_ACCESS_MODE_WRITE)==0);
            }
        }

        SECTION("should return false when no routers present") {
            // make it non-expired
            uint64_t time = BoltTime_get_time_ms();
            table->expires = time+5000;
            table->last_updated = time;

            // add reader & writer
            BoltAddressSet_add(table->writers, server1);
            BoltAddressSet_add(table->readers, server2);

            SECTION("READ") {
                REQUIRE(RoutingTable_is_expired(table, BOLT_ACCESS_MODE_READ)==1);
            }

            SECTION("WRITE") {
                REQUIRE(RoutingTable_is_expired(table, BOLT_ACCESS_MODE_WRITE)==1);
            }
        }

        SECTION("should return false when requested server type is not present") {
            // make it non-expired
            uint64_t time = BoltTime_get_time_ms();
            table->expires = time+5000;
            table->last_updated = time;

            // add router
            BoltAddressSet_add(table->routers, server1);

            SECTION("READ") {
                REQUIRE(RoutingTable_is_expired(table, BOLT_ACCESS_MODE_READ)==1);
            }

            SECTION("WRITE") {
                REQUIRE(RoutingTable_is_expired(table, BOLT_ACCESS_MODE_WRITE)==1);
            }
        }

        RoutingTable_destroy(table);
    }

    SECTION("RoutingTable_forget_server") {
        volatile RoutingTable* table = RoutingTable_create();

        SECTION("should have no effect on empty table") {
            REQUIRE(BoltAddressSet_size(table->readers)==0);
            REQUIRE(BoltAddressSet_size(table->writers)==0);
            REQUIRE(BoltAddressSet_size(table->routers)==0);

            RoutingTable_forget_server(table, server1);

            REQUIRE(BoltAddressSet_size(table->readers)==0);
            REQUIRE(BoltAddressSet_size(table->writers)==0);
            REQUIRE(BoltAddressSet_size(table->routers)==0);
        }

        SECTION("should have no effect with non-existent servers") {
            BoltAddressSet_add(table->readers, server1);
            BoltAddressSet_add(table->readers, server2);
            BoltAddressSet_add(table->writers, server3);
            BoltAddressSet_add(table->writers, server4);
            BoltAddressSet_add(table->routers, server5);
            BoltAddressSet_add(table->routers, server6);

            REQUIRE(BoltAddressSet_size(table->readers)==2);
            REQUIRE(BoltAddressSet_size(table->writers)==2);
            REQUIRE(BoltAddressSet_size(table->routers)==2);

            RoutingTable_forget_server(table, server7);

            REQUIRE(BoltAddressSet_size(table->readers)==2);
            REQUIRE(BoltAddressSet_size(table->writers)==2);
            REQUIRE(BoltAddressSet_size(table->routers)==2);
        }

        SECTION("should remove server from all server sets") {
            BoltAddressSet_add(table->readers, server1);
            BoltAddressSet_add(table->readers, server2);
            BoltAddressSet_add(table->writers, server1);
            BoltAddressSet_add(table->writers, server3);
            BoltAddressSet_add(table->routers, server1);
            BoltAddressSet_add(table->routers, server4);

            REQUIRE(BoltAddressSet_size(table->readers)==2);
            REQUIRE(BoltAddressSet_size(table->writers)==2);
            REQUIRE(BoltAddressSet_size(table->routers)==2);

            RoutingTable_forget_server(table, server1);

            REQUIRE(BoltAddressSet_size(table->readers)==1);
            REQUIRE(BoltAddressSet_index_of(table->readers, server1)==-1);
            REQUIRE(BoltAddressSet_index_of(table->readers, server2)==0);
            REQUIRE(BoltAddressSet_size(table->writers)==1);
            REQUIRE(BoltAddressSet_index_of(table->writers, server1)==-1);
            REQUIRE(BoltAddressSet_index_of(table->writers, server3)==0);
            REQUIRE(BoltAddressSet_size(table->routers)==1);
            REQUIRE(BoltAddressSet_index_of(table->routers, server1)==-1);
            REQUIRE(BoltAddressSet_index_of(table->routers, server4)==0);
        }

        RoutingTable_destroy(table);
    }

    SECTION("RoutingTable_forget_writer") {
        volatile RoutingTable* table = RoutingTable_create();

        SECTION("should have no effect on empty writers") {
            REQUIRE(BoltAddressSet_size(table->writers)==0);

            RoutingTable_forget_writer(table, server1);

            REQUIRE(BoltAddressSet_size(table->writers)==0);
        }

        SECTION("should have no effect with non-existent writers") {
            BoltAddressSet_add(table->writers, server1);
            BoltAddressSet_add(table->writers, server2);
            BoltAddressSet_add(table->readers, server3);
            BoltAddressSet_add(table->readers, server4);
            BoltAddressSet_add(table->routers, server5);
            BoltAddressSet_add(table->routers, server6);

            REQUIRE(BoltAddressSet_size(table->readers)==2);
            REQUIRE(BoltAddressSet_size(table->writers)==2);
            REQUIRE(BoltAddressSet_size(table->routers)==2);

            RoutingTable_forget_writer(table, server7);

            REQUIRE(BoltAddressSet_size(table->readers)==2);
            REQUIRE(BoltAddressSet_size(table->writers)==2);
            REQUIRE(BoltAddressSet_size(table->routers)==2);
        }

        SECTION("should remove server from writers") {
            BoltAddressSet_add(table->readers, server1);
            BoltAddressSet_add(table->readers, server2);
            BoltAddressSet_add(table->writers, server1);
            BoltAddressSet_add(table->writers, server3);
            BoltAddressSet_add(table->routers, server1);
            BoltAddressSet_add(table->routers, server4);

            REQUIRE(BoltAddressSet_size(table->readers)==2);
            REQUIRE(BoltAddressSet_size(table->writers)==2);
            REQUIRE(BoltAddressSet_size(table->routers)==2);

            RoutingTable_forget_writer(table, server1);

            REQUIRE(BoltAddressSet_size(table->readers)==2);
            REQUIRE(BoltAddressSet_index_of(table->readers, server1)==0);
            REQUIRE(BoltAddressSet_index_of(table->readers, server2)==1);
            REQUIRE(BoltAddressSet_size(table->writers)==1);
            REQUIRE(BoltAddressSet_index_of(table->writers, server1)==-1);
            REQUIRE(BoltAddressSet_index_of(table->writers, server3)==0);
            REQUIRE(BoltAddressSet_size(table->routers)==2);
            REQUIRE(BoltAddressSet_index_of(table->readers, server1)==0);
            REQUIRE(BoltAddressSet_index_of(table->readers, server2)==1);
        }

        RoutingTable_destroy(table);
    }

    BoltAddress_destroy(server1);
    BoltAddress_destroy(server2);
    BoltAddress_destroy(server3);
    BoltAddress_destroy(server4);
    BoltAddress_destroy(server5);
    BoltAddress_destroy(server6);
    BoltAddress_destroy(server7);
}
