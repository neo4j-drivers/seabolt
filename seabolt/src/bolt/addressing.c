/*
 * Copyright (c) 2002-2018 "Neo Technology,"
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


#include "bolt/addressing.h"
#include "bolt/logging.h"
#include "bolt/mem.h"
#include "memory.h"
#include "bolt/config-impl.h"


#define SOCKADDR_STORAGE_SIZE sizeof(struct sockaddr_storage)


struct BoltAddress* BoltAddress_create(const char * host, const char * port)
{
    struct BoltAddress* service = BoltMem_allocate(sizeof(struct BoltAddress));
    service->host = host;
    service->port = port;
    service->n_resolved_hosts = 0;
    service->resolved_hosts = BoltMem_allocate(0);
    service->resolved_port = 0;
    return service;
}

int BoltAddress_resolve_b(struct BoltAddress * address)
{
    if (strchr(address->host, ':') == NULL)
    {
        BoltLog_info("bolt: Resolving address %s:%s", address->host, address->port);
    }
    else
    {
        BoltLog_info("bolt: Resolving address [%s]:%s", address->host, address->port);
    }
    static struct addrinfo hints;
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = (AI_V4MAPPED | AI_ADDRCONFIG);
    struct addrinfo* ai;
    int gai_status = getaddrinfo(address->host, address->port, &hints, &ai);
    if (gai_status == 0)
    {
        unsigned short n_resolved = 0;
        for (struct addrinfo* ai_node = ai; ai_node != NULL; ai_node = ai_node->ai_next)
        {
            switch (ai_node->ai_family)
            {
                case AF_INET:
                case AF_INET6:
                {
                    n_resolved += 1;
                    break;
                }
                default:
                    // We only care about IPv4 and IPv6
                    continue;
            }
        }
        address->resolved_hosts = BoltMem_reallocate(address->resolved_hosts,
                                                     address->n_resolved_hosts * SOCKADDR_STORAGE_SIZE, n_resolved * SOCKADDR_STORAGE_SIZE);
        address->n_resolved_hosts = n_resolved;
        size_t p = 0;
        for (struct addrinfo* ai_node = ai; ai_node != NULL; ai_node = ai_node->ai_next)
        {
            switch (ai_node->ai_family)
            {
                case AF_INET:
                case AF_INET6:
                {
                    struct sockaddr_storage * target = &address->resolved_hosts[p];
                    memcpy(target, ai_node->ai_addr, ai_node->ai_addrlen);
                    break;
                }
                default:
                    // We only care about IPv4 and IPv6
                    continue;
            }
            p += 1;
        }
        freeaddrinfo(ai);
        if (address->n_resolved_hosts == 1)
        {
            BoltLog_info("bolt: Host resolved to 1 IP address");
        }
        else
        {
            BoltLog_info("bolt: Host resolved to %d IP addresses", address->n_resolved_hosts);
        }
    }
    else
    {
        BoltLog_info("bolt: Host resolution failed (status %d)", gai_status);
    }

    if (address->n_resolved_hosts > 0)
    {
        struct sockaddr_storage *resolved = &address->resolved_hosts[0];
        in_port_t resolved_port = resolved->ss_family == AF_INET ?
                                  ((struct sockaddr_in *)resolved)->sin_port : ((struct sockaddr_in6 *)resolved)->sin6_port;
        address->resolved_port = ntohs(resolved_port);
    }

    return gai_status;
}

int BoltAddress_copy_resolved_host(struct BoltAddress * address, size_t index, char * buffer,
                                   socklen_t buffer_size)
{
    struct sockaddr_storage * resolved_host = &address->resolved_hosts[index];
    int status = getnameinfo((const struct sockaddr *)resolved_host, SOCKADDR_STORAGE_SIZE, buffer, buffer_size, NULL, 0, NI_NUMERICHOST);
    switch (status)
    {
        case 0:
            return resolved_host->ss_family;
        default:
            return -1;
    }
}

void BoltAddress_destroy(struct BoltAddress * address)
{
    BoltMem_deallocate(address->resolved_hosts, address->n_resolved_hosts * SOCKADDR_STORAGE_SIZE);
    BoltMem_deallocate(address, sizeof(struct BoltAddress));
}
