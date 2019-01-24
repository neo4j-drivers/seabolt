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

#include <string.h>

#include "bolt-private.h"
#include "address-private.h"
#include "log-private.h"
#include "mem.h"
#include "name.h"
#include "sync.h"

#define DEFAULT_BOLT_PORT "7687"
#define DEFAULT_BOLT_HOST "localhost"

#define SOCKADDR_STORAGE_SIZE sizeof(struct sockaddr_storage)

BoltAddress* BoltAddress_create(const char* host, const char* port)
{
    BoltAddress* address = (BoltAddress*) BoltMem_allocate(sizeof(BoltAddress));

    if (host==NULL || strlen(host)==0) {
        host = DEFAULT_BOLT_HOST;
    }
    address->host = (char*) BoltMem_duplicate(host, strlen(host)+1);

    if (port==NULL || strlen(port)==0) {
        port = DEFAULT_BOLT_PORT;
    }
    address->port = (char*) BoltMem_duplicate(port, strlen(port)+1);
    address->n_resolved_hosts = 0;
    address->resolved_hosts = NULL;
    address->resolved_port = 0;
    address->lock = NULL;
    return address;
}

BoltAddress* BoltAddress_create_with_lock(const char* host, const char* port)
{
    BoltAddress* address = BoltAddress_create(host, port);
    BoltSync_mutex_create(&address->lock);
    return address;
}

const char* BoltAddress_host(BoltAddress* address)
{
    return address->host;
}

const char* BoltAddress_port(BoltAddress* address)
{
    return address->port;
}

BoltAddress* BoltAddress_create_from_string(const char* endpoint_str, uint64_t endpoint_len)
{
    // Create a copy of the string and add null character at the end to properly
    // work with string functions
    char* address_str = (char*) BoltMem_duplicate(endpoint_str, endpoint_len+1);
    address_str[endpoint_len] = '\0';

    // Find the last index of `:` which is our port separator
    char* port_str = strrchr(address_str, ':');
    if (port_str!=NULL) {
        // If we found the separator, set it to null and advance the pointer by 1
        // By this we have two null terminated strings pointed by address_str (hostname)
        // port_str (port)
        *port_str++ = '\0';
    }

    BoltAddress* result = BoltAddress_create(address_str, port_str);
    BoltMem_deallocate(address_str, endpoint_len+1);
    return result;
}

int32_t BoltAddress_resolve(BoltAddress* address, int32_t* n_resolved, BoltLog* log)
{
    if (address->lock!=NULL) {
        BoltSync_mutex_lock(&address->lock);
    }

    if (strchr(address->host, ':')==NULL) {
        BoltLog_info(log, "[addr]: Resolving address %s:%s", address->host, address->port);
    }
    else {
        BoltLog_info(log, "[addr]: Resolving address [%s]:%s", address->host, address->port);
    }
    static struct addrinfo hints;
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = (AI_V4MAPPED | AI_ADDRCONFIG);
    struct addrinfo* ai;
    const int gai_status = getaddrinfo(address->host, address->port, &hints, &ai);
    if (gai_status==0) {
        unsigned short count = 0;
        for (struct addrinfo* ai_node = ai; ai_node!=NULL; ai_node = ai_node->ai_next) {
            switch (ai_node->ai_family) {
            case AF_INET:
            case AF_INET6: {
                count += 1;
                break;
            }
            default:
                // We only care about IPv4 and IPv6
                continue;
            }
        }
        if (address->resolved_hosts==NULL) {
            address->resolved_hosts = (struct sockaddr_storage*) BoltMem_allocate(count*SOCKADDR_STORAGE_SIZE);
        }
        else {
            address->resolved_hosts = (struct sockaddr_storage*) BoltMem_reallocate(address->resolved_hosts,
                    address->n_resolved_hosts*SOCKADDR_STORAGE_SIZE,
                    count*SOCKADDR_STORAGE_SIZE);
        }
        address->n_resolved_hosts = count;
        size_t p = 0;
        for (struct addrinfo* ai_node = ai; ai_node!=NULL; ai_node = ai_node->ai_next) {
            switch (ai_node->ai_family) {
            case AF_INET:
            case AF_INET6: {
                struct sockaddr_storage* target = &address->resolved_hosts[p];
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
        if (address->n_resolved_hosts==1) {
            BoltLog_info(log, "[addr]: Host resolved to 1 IP address");
        }
        else {
            BoltLog_info(log, "[addr]: Host resolved to %d IP addresses", address->n_resolved_hosts);
        }
    }
    else {
        BoltLog_info(log, "[addr]: Host resolution failed (status %d)", gai_status);
    }

    if (address->n_resolved_hosts>0) {
        struct sockaddr_storage* resolved = &address->resolved_hosts[0];
        const uint16_t resolved_port = resolved->ss_family==AF_INET ?
                                       ((struct sockaddr_in*) (resolved))->sin_port
                                                                    : ((struct sockaddr_in6*) (resolved))->sin6_port;
        address->resolved_port = ntohs(resolved_port);
    }

    if (gai_status==0 && n_resolved!=NULL) {
        *n_resolved = address->n_resolved_hosts;
    }

    if (address->lock!=NULL) {
        BoltSync_mutex_unlock(&address->lock);
    }

    return gai_status;
}

int32_t BoltAddress_copy_resolved_host(BoltAddress* address, int32_t index, char* buffer,
        uint64_t buffer_size)
{
    struct sockaddr_storage* resolved_host = &address->resolved_hosts[index];
    int status = get_address_components(resolved_host, buffer, buffer_size, NULL, 0);
    switch (status) {
    case 0:
        return resolved_host->ss_family;
    default:
        return -1;
    }
}

void BoltAddress_destroy(BoltAddress* address)
{
    if (address->resolved_hosts!=NULL) {
        address->resolved_hosts = (struct sockaddr_storage*) BoltMem_deallocate(address->resolved_hosts,
                address->n_resolved_hosts*SOCKADDR_STORAGE_SIZE);
        address->n_resolved_hosts = 0;
    }

    if (address->lock!=NULL) {
        BoltSync_mutex_destroy(&address->lock);
    }

    BoltMem_deallocate((char*) address->host, strlen(address->host)+1);
    BoltMem_deallocate((char*) address->port, strlen(address->port)+1);
    BoltMem_deallocate(address, sizeof(BoltAddress));
}
