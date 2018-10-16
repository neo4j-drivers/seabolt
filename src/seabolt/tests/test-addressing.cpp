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


#include "integration.hpp"
#include "catch.hpp"

#if USE_WINSOCK
#include <winsock2.h>
#endif

SCENARIO("Test address construction", "")
{
    WHEN("hostname is provided as NULL")
    {
        struct BoltAddress* address = BoltAddress_create(nullptr, "7687");

        REQUIRE(address->host!=nullptr);
        REQUIRE(strcmp(address->host, "localhost")==0);

        BoltAddress_destroy(address);
    }

    WHEN("hostname is provided as empty string")
    {
        struct BoltAddress* address = BoltAddress_create("", "7687");

        REQUIRE(address->host!=nullptr);
        REQUIRE(strcmp(address->host, "localhost")==0);

        BoltAddress_destroy(address);
    }

    WHEN("port is provided as NULL")
    {
        struct BoltAddress* address = BoltAddress_create("localhost", nullptr);

        REQUIRE(address->port!=nullptr);
        REQUIRE(strcmp(address->port, "7687")==0);

        BoltAddress_destroy(address);
    }

    WHEN("port is provided as empty string")
    {
        struct BoltAddress* address = BoltAddress_create("localhost", "");

        REQUIRE(address->port!=nullptr);
        REQUIRE(strcmp(address->port, "7687")==0);

        BoltAddress_destroy(address);
    }

    WHEN("hostname is provided")
    {
        struct BoltAddress* address = BoltAddress_create("some.host.name", "7687");

        REQUIRE(address->host!=nullptr);
        REQUIRE(strcmp(address->host, "some.host.name")==0);

        BoltAddress_destroy(address);
    }

    WHEN("port is provided")
    {
        struct BoltAddress* address = BoltAddress_create("localhost", "1578");

        REQUIRE(address->port!=nullptr);
        REQUIRE(strcmp(address->port, "1578")==0);

        BoltAddress_destroy(address);
    }
}

SCENARIO("Test address resolution (IPv4)", "[dns]")
{
    const char* host = "ipv4-only.bolt-test.net";
    const char* port = "7687";
    struct BoltAddress* address = BoltAddress_create(host, port);
    REQUIRE(strcmp(address->host, host)==0);
    REQUIRE(strcmp(address->port, port)==0);
    REQUIRE(address->n_resolved_hosts==0);
    REQUIRE(address->resolved_port==0);
    for (int i = 0; i<2; i++) {
        BoltAddress_resolve(address, nullptr);
        REQUIRE(address->n_resolved_hosts==1);
        char host_string[40];
        int af = BoltAddress_copy_resolved_host(address, 0, &host_string[0], sizeof(host_string));
        REQUIRE(af==AF_INET);
        REQUIRE(strcmp(host_string, "52.215.65.80")==0);
        REQUIRE(address->resolved_port==7687);
    }
    BoltAddress_destroy(address);
}

SCENARIO("Test address resolution (IPv6)", "[dns]")
{
    const char* host = "ipv6-only.bolt-test.net";
    const char* port = "7687";
    struct BoltAddress* address = BoltAddress_create(host, port);
    REQUIRE(strcmp(address->host, host)==0);
    REQUIRE(strcmp(address->port, port)==0);
    REQUIRE(address->n_resolved_hosts==0);
    REQUIRE(address->resolved_port==0);
    for (int i = 0; i<2; i++) {
        int status = BoltAddress_resolve(address, nullptr);
        if (status==0) {
            REQUIRE(address->n_resolved_hosts==1);
            char host_string[40];
            int af = BoltAddress_copy_resolved_host(address, 0, &host_string[0], sizeof(host_string));
            REQUIRE(af==AF_INET6);
            REQUIRE(strcmp(host_string, "2a05:d018:1ca:6113:c9d8:4689:33f2:15f7")==0);
            REQUIRE(address->resolved_port==7687);
        }
    }
    BoltAddress_destroy(address);
}

SCENARIO("Test address resolution (IPv4 and IPv6)", "[dns]")
{
    const char* host = "ipv4-and-ipv6.bolt-test.net";
    const char* port = "7687";
    struct BoltAddress* address = BoltAddress_create(host, port);
    REQUIRE(strcmp(address->host, host)==0);
    REQUIRE(strcmp(address->port, port)==0);
    REQUIRE(address->n_resolved_hosts==0);
    REQUIRE(address->resolved_port==0);
    for (int i = 0; i<2; i++) {
        int status = BoltAddress_resolve(address, nullptr);
        if (status==0) {
            for (size_t j = 0; j<address->n_resolved_hosts; j++) {
                char host_string[40];
                int af = BoltAddress_copy_resolved_host(address, j, &host_string[0], sizeof(host_string));
                switch (af) {
                case AF_INET:
                    REQUIRE(strcmp(host_string, "52.215.65.80")==0);
                    break;
                case AF_INET6:
                    REQUIRE(strcmp(host_string, "2a05:d018:1ca:6113:c9d8:4689:33f2:15f7")==0);
                    break;
                default:
                    FAIL();
                }
            }
            REQUIRE(address->resolved_port==7687);
        }
    }
    BoltAddress_destroy(address);
}

