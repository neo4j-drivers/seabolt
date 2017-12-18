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


struct Bolt
{
    struct BoltConnection* connection;
    enum BoltTransport transport;
    struct addrinfo* address;
    const char* user;
    const char* password;
    struct
    {
        struct timespec connect_time;
        struct timespec init_time;
    } stats;
};

const char* getenv_or_default(const char* name, const char* default_value)
{
    const char* value = getenv(name);
    return (value == NULL) ? default_value : value;
}

struct Bolt* Bolt_create(int argc, char* argv[])
{
    const char* BOLT_SECURE = getenv_or_default("BOLT_SECURE", "1");
    const char* BOLT_HOST = getenv_or_default("BOLT_HOST", "localhost");
    const char* BOLT_PORT = getenv_or_default("BOLT_PORT", "7687");
    const char* BOLT_USER = getenv_or_default("BOLT_USER", "neo4j");
    const char* BOLT_PASSWORD = getenv("BOLT_PASSWORD");

    struct Bolt* bolt = BoltMem_allocate(sizeof(struct Bolt));
    bolt->transport = (strcmp(BOLT_SECURE, "1") == 0) ? BOLT_SECURE_SOCKET : BOLT_SOCKET;

    int res = getaddrinfo(BOLT_HOST, BOLT_PORT, NULL, &bolt->address);
    if (res != 0)
    {
        BoltLog_error("[NET] Could not resolve address '%s' for port '%s' (error %d)", BOLT_HOST, BOLT_PORT, res);
        return NULL;
    }

    bolt->user = BOLT_USER;
    bolt->password = BOLT_PASSWORD;

    return bolt;
}

void Bolt_destroy(struct Bolt* bolt)
{
    freeaddrinfo(bolt->address);
    BoltMem_deallocate(bolt, sizeof(struct Bolt));
}

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

int Bolt_connect(struct Bolt* bolt)
{
    struct timespec t[2];
    timespec_get(&t[0], TIME_UTC);
    bolt->connection = BoltConnection_open_b(bolt->transport, bolt->address);
    timespec_get(&t[1], TIME_UTC);
    timespec_diff(&bolt->stats.connect_time, &t[1], &t[0]);
}

int Bolt_dump_received(struct Bolt* bolt)
{
    BoltValue_write(stdout, bolt->connection->received, bolt->connection->protocol_version);
    fprintf(stdout, "\n");
}

int Bolt_init(struct Bolt* bolt)
{
    struct timespec t[2];
    timespec_get(&t[0], TIME_UTC);
    BoltConnection_init_b(bolt->connection, bolt->user, bolt->password);
    Bolt_dump_received(bolt);
    timespec_get(&t[1], TIME_UTC);
    timespec_diff(&bolt->stats.init_time, &t[1], &t[0]);
}

int Bolt_run(struct Bolt* bolt, const char* statement)
{
    struct timespec t[7];

    timespec_get(&t[1], TIME_UTC);    // Checkpoint 1 - right at the start

    Bolt_connect(bolt);
    assert(bolt->connection->status == BOLT_CONNECTED);

    Bolt_init(bolt);
    assert(bolt->connection->status == BOLT_READY);

    timespec_get(&t[2], TIME_UTC);    // Checkpoint 2 - after handshake and initialisation

    BoltValue_to_UTF8(bolt->connection->cypher_statement, statement, (int32_t)(strlen(statement)));
    BoltUTF8Dictionary_resize(bolt->connection->cypher_parameters, 1);
    BoltUTF8Dictionary_with_key(bolt->connection->cypher_parameters, 0, "x", 1);

    BoltValue_to_Int32(BoltUTF8Dictionary_value(bolt->connection->cypher_parameters, 0), 1234);
    BoltConnection_load_run(bolt->connection);
    BoltConnection_load_pull(bolt->connection, -1);

    BoltValue_to_Float64(BoltUTF8Dictionary_value(bolt->connection->cypher_parameters, 0), 3.1415);
    BoltConnection_load_run(bolt->connection);
    BoltConnection_load_pull(bolt->connection, -1);

    BoltValue_to_UTF8(BoltUTF8Dictionary_value(bolt->connection->cypher_parameters, 0), "three", 5);
    BoltConnection_load_run(bolt->connection);
    BoltConnection_load_pull(bolt->connection, -1);

    BoltConnection_transmit_b(bolt->connection);

    timespec_get(&t[3], TIME_UTC);    // Checkpoint 3 - after query transmission

    long record_count = 0;
    for (int i = 0; i < 3; i++)
    {

        BoltConnection_receive_summary_b(bolt->connection);
        Bolt_dump_received(bolt);

        timespec_get(&t[4], TIME_UTC);    // Checkpoint 4 - receipt of header

        while (BoltConnection_receive_b(bolt->connection))
        {
            Bolt_dump_received(bolt);
            record_count += 1;
        }
        Bolt_dump_received(bolt);

    }

    timespec_get(&t[5], TIME_UTC);    // Checkpoint 5 - receipt of footer

    BoltConnection_close_b(bolt->connection);

    timespec_get(&t[6], TIME_UTC);    // Checkpoint 6 - after close

    ///////////////////////////////////////////////////////////////////

    fprintf(stderr, "query                : %s\n", statement);
    fprintf(stderr, "record count         : %ld\n", record_count);

    timespec_diff(&t[0], &t[2], &t[1]);
    fprintf(stderr, "initialisation       : %lds %09ldns\n", t[0].tv_sec, t[0].tv_nsec);

    timespec_diff(&t[0], &t[3], &t[2]);
    fprintf(stderr, "query transmission   : %lds %09ldns\n", t[0].tv_sec, t[0].tv_nsec);

    timespec_diff(&t[0], &t[4], &t[3]);
    fprintf(stderr, "query processing     : %lds %09ldns\n", t[0].tv_sec, t[0].tv_nsec);

    timespec_diff(&t[0], &t[5], &t[4]);
    fprintf(stderr, "result processing    : %lds %09ldns\n", t[0].tv_sec, t[0].tv_nsec);

    timespec_diff(&t[0], &t[6], &t[5]);
    fprintf(stderr, "shutdown             : %lds %09ldns\n", t[0].tv_sec, t[0].tv_nsec);

    timespec_diff(&t[0], &t[6], &t[1]);
    fprintf(stderr, "=====================================\n");
    fprintf(stderr, "TOTAL                : %lds %09ldns\n", t[0].tv_sec, t[0].tv_nsec);

}

int main(int argc, char* argv[])
{
    struct Bolt* bolt = Bolt_create(argc, argv);

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
        Bolt_run(bolt, argv[1]);
    }
    else
    {
        Bolt_run(bolt, "RETURN 1");
    }

    Bolt_destroy(bolt);
}
