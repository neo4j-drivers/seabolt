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

#include <netdb.h>
#include <stdlib.h>
#include <time.h>
#include <memory.h>
#include <assert.h>

#include "bolt.h"


void timespec_diff(struct timespec* t, struct timespec* t0, struct timespec* t1)
{
    t->tv_sec = t0->tv_sec - t1->tv_sec;
    t->tv_nsec = t0->tv_nsec - t1->tv_nsec;
    while (t->tv_nsec >= 1000000000)
    {
        t->tv_sec += 1;
        t->tv_nsec -= 1000000000;
    }
    while (t->tv_nsec < 0)
    {
        t->tv_sec -= 1;
        t->tv_nsec += 1000000000;
    }
}

const char* getenv_or_default(const char* name, const char* default_value)
{
    const char* value = getenv(name);
    return (value == NULL) ? default_value : value;
}

int run(const char* statement)
{
    const char* BOLT_SECURE = getenv_or_default("BOLT_SECURE", "1");
    const char* BOLT_HOST = getenv_or_default("BOLT_HOST", "localhost");
    const char* BOLT_PORT = getenv_or_default("BOLT_PORT", "7687");
    const char* BOLT_USER = getenv_or_default("BOLT_USER", "neo4j");
    const char* BOLT_PASSWORD = getenv("BOLT_PASSWORD");

    struct timespec t[7];

    timespec_get(&t[1], TIME_UTC);    // Checkpoint 1 - right at the start

    struct BoltConnection* connection;

    struct addrinfo* address;
    int res = getaddrinfo(BOLT_HOST, BOLT_PORT, NULL, &address);
    if (res != 0)
    {
        BoltLog_error("[NET] Could not resolve address '%s' for port '%s' (error %d)", BOLT_HOST, BOLT_PORT, res);
        return -1;
    }
    if (strcmp(BOLT_SECURE, "1") == 0)
    {
        connection = BoltConnection_open_b(BOLT_SECURE_SOCKET, address);
    }
    else
    {
        connection = BoltConnection_open_b(BOLT_SOCKET, address);
    }
    freeaddrinfo(address);
    assert(connection->status == BOLT_CONNECTED);

    BoltConnection_init_b(connection, BOLT_USER, BOLT_PASSWORD);
    struct BoltValue* current = BoltConnection_current(connection);
    BoltValue_write(stderr, current);
    fprintf(stderr, "\n");
    assert(connection->status == BOLT_READY);

    timespec_get(&t[2], TIME_UTC);    // Checkpoint 2 - after handshake and initialisation

    BoltConnection_load_run(connection, statement);
    BoltConnection_load_pull(connection, -1);
    BoltConnection_transmit_b(connection);

    timespec_get(&t[3], TIME_UTC);    // Checkpoint 3 - after query transmission

    BoltConnection_receive_b(connection);
    current = BoltConnection_current(connection);
    BoltValue_write(stderr, current);
    fprintf(stderr, "\n");

    timespec_get(&t[4], TIME_UTC);    // Checkpoint 4 - receipt of header

    long record_count = 0;
    while (BoltConnection_fetch_b(connection))
    {
        current = BoltConnection_current(connection);
        BoltValue_write(stdout, current);
        fprintf(stdout, "\n");
        record_count += 1;
    }
    BoltValue_write(stderr, current);
    fprintf(stderr, "\n");

    timespec_get(&t[5], TIME_UTC);    // Checkpoint 5 - receipt of footer

    BoltConnection_close_b(connection);

    timespec_get(&t[6], TIME_UTC);    // Checkpoint 6 - after close

    ///////////////////////////////////////////////////////////////////

    printf("query                : %s\n", statement);
    printf("record count         : %ld\n", record_count);

    timespec_diff(&t[0], &t[2], &t[1]);
    printf("initialisation       : %lds %09ldns\n", t[0].tv_sec, t[0].tv_nsec);

    timespec_diff(&t[0], &t[3], &t[2]);
    printf("query transmission   : %lds %09ldns\n", t[0].tv_sec, t[0].tv_nsec);

    timespec_diff(&t[0], &t[4], &t[3]);
    printf("query processing     : %lds %09ldns\n", t[0].tv_sec, t[0].tv_nsec);

    timespec_diff(&t[0], &t[5], &t[4]);
    printf("result processing    : %lds %09ldns\n", t[0].tv_sec, t[0].tv_nsec);

    timespec_diff(&t[0], &t[6], &t[5]);
    printf("shutdown             : %lds %09ldns\n", t[0].tv_sec, t[0].tv_nsec);

    timespec_diff(&t[0], &t[6], &t[1]);
    printf("=====================================\n");
    printf("TOTAL                : %lds %09ldns\n", t[0].tv_sec, t[0].tv_nsec);

}

int main(int argc, char* argv[])
{
    const char* BOLT_LOG = getenv_or_default("BOLT_LOG", "0");
    if (strcmp(BOLT_LOG, "1") == 0)
    {
        BoltLog_setFile(stdout);
    }
    if (strcmp(BOLT_LOG, "2") == 0)
    {
        BoltLog_setFile(stderr);
    }
    else
    {
        BoltLog_setFile(NULL);
    }
    if (argc >= 2)
    {
        run(argv[1]);
    }
    else
    {
        run("RETURN 1");
    }
}
