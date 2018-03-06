/*
 * Copyright (c) 2002-2017 "Neo Technology,"
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


#include "integration.hpp"
#include "catch.hpp"


SCENARIO("Test address resolution (IPv4)", "[dns]")
{
    const char * host = "ipv4-only.bolt-test.net";
    const char * port = "7687";
    struct BoltAddress * address = BoltAddress_create(host, port);
    REQUIRE(strcmp(address->host, host) == 0);
    REQUIRE(strcmp(address->port, port) == 0);
    REQUIRE(address->n_resolved_hosts == 0);
    REQUIRE(address->resolved_port == 0);
    for (int i = 0; i < 2; i++)
    {
        BoltAddress_resolve_b(address);
        REQUIRE(address->n_resolved_hosts == 1);
        char host_string[40];
        int af = BoltAddress_copy_resolved_host(address, 0, &host_string[0], sizeof(host_string));
        REQUIRE(af == AF_INET);
        REQUIRE(strcmp(host_string, "52.215.65.80") == 0);
        REQUIRE(address->resolved_port == 7687);
    }
    BoltAddress_destroy(address);
}

SCENARIO("Test address resolution (IPv6)", "[dns]")
{
    const char * host = "ipv6-only.bolt-test.net";
    const char * port = "7687";
    struct BoltAddress * address = BoltAddress_create(host, port);
    REQUIRE(strcmp(address->host, host) == 0);
    REQUIRE(strcmp(address->port, port) == 0);
    REQUIRE(address->n_resolved_hosts == 0);
    REQUIRE(address->resolved_port == 0);
    for (int i = 0; i < 2; i++)
    {
        int status = BoltAddress_resolve_b(address);
        if (status == 0)
        {
            REQUIRE(address->n_resolved_hosts == 1);
            char host_string[40];
            int af = BoltAddress_copy_resolved_host(address, 0, &host_string[0], sizeof(host_string));
            REQUIRE(af == AF_INET6);
            REQUIRE(strcmp(host_string, "2a05:d018:1ca:6113:c9d8:4689:33f2:15f7") == 0);
            REQUIRE(address->resolved_port == 7687);
        }
    }
    BoltAddress_destroy(address);
}

SCENARIO("Test address resolution (IPv4 and IPv6)", "[dns]")
{
    const char * host = "ipv4-and-ipv6.bolt-test.net";
    const char * port = "7687";
    struct BoltAddress * address = BoltAddress_create(host, port);
    REQUIRE(strcmp(address->host, host) == 0);
    REQUIRE(strcmp(address->port, port) == 0);
    REQUIRE(address->n_resolved_hosts == 0);
    REQUIRE(address->resolved_port == 0);
    for (int i = 0; i < 2; i++)
    {
        int status = BoltAddress_resolve_b(address);
        if (status == 0)
        {
            for (size_t j = 0; j < address->n_resolved_hosts; j++)
            {
                char host_string[40];
                int af = BoltAddress_copy_resolved_host(address, j, &host_string[0], sizeof(host_string));
                switch (af)
                {
                    case AF_INET:
                        REQUIRE(strcmp(host_string, "52.215.65.80") == 0);
                        break;
                    case AF_INET6:
                        REQUIRE(strcmp(host_string, "2a05:d018:1ca:6113:c9d8:4689:33f2:15f7") == 0);
                        break;
                    default:
                        FAIL();
                }
            }
            REQUIRE(address->resolved_port == 7687);
        }
    }
    BoltAddress_destroy(address);
}
