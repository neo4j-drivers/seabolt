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

#include <stdlib.h>
#include <time.h>
#include <memory.h>
#include <assert.h>
#include <stdint.h>

#include "bolt/lifecycle.h"
#include "bolt/connect.h"
#include "bolt/mem.h"
#include "bolt/values.h"
#include "bolt/logging.h"

#ifdef WIN32
#include <winsock2.h>
#endif // WIN32

#ifdef __APPLE__
#include <mach/clock.h>
#include <mach/mach.h>

#define TIME_UTC 0

void timespec_get(struct timespec *ts, int type)
{
    clock_serv_t cclock;
    mach_timespec_t mts;
    host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &cclock);
    clock_get_time(cclock, &mts);
    mach_port_deallocate(mach_task_self(), cclock);
    ts->tv_sec = mts.tv_sec;
    ts->tv_nsec = mts.tv_nsec;
}
#endif

struct BoltApplication
{
    struct BoltConnection* connection;
    enum BoltTransport transport;
    struct BoltAddress* address;
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

struct BoltApplication* BoltApplication_create()
{
    const char* BOLT_SECURE = getenv_or_default("BOLT_SECURE", "1");
    const char* BOLT_HOST = getenv_or_default("BOLT_HOST", "localhost");
    const char* BOLT_PORT = getenv_or_default("BOLT_PORT", "7687");
    const char* BOLT_USER = getenv_or_default("BOLT_USER", "neo4j");
    const char* BOLT_PASSWORD = getenv("BOLT_PASSWORD");

    struct BoltApplication * app = BoltMem_allocate(sizeof(struct BoltApplication));
    app->transport = (strcmp(BOLT_SECURE, "1") == 0) ? BOLT_SECURE_SOCKET : BOLT_INSECURE_SOCKET;
    app->address = BoltAddress_create(BOLT_HOST, BOLT_PORT);
    BoltAddress_resolve_b(app->address);
    app->user = BOLT_USER;
    app->password = BOLT_PASSWORD;

    return app;
}

void BoltApplication_destroy(struct BoltApplication * bolt)
{
    BoltAddress_destroy(bolt->address);
    BoltMem_deallocate(bolt, sizeof(struct BoltApplication));
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

int BoltApplication_connect(struct BoltApplication * bolt)
{
    struct timespec t[2];
    timespec_get(&t[0], TIME_UTC);
    bolt->connection = BoltConnection_open_b(bolt->transport, bolt->address);
    timespec_get(&t[1], TIME_UTC);
    timespec_diff(&bolt->stats.connect_time, &t[1], &t[0]);
    return 0;
}

int BoltApplication_init(struct BoltApplication * bolt)
{
    struct timespec t[2];
    timespec_get(&t[0], TIME_UTC);
    BoltConnection_init_b(bolt->connection, "seabolt/1.0.0a", bolt->user, bolt->password);
//    Bolt_dump_last_received(bolt);
    timespec_get(&t[1], TIME_UTC);
    timespec_diff(&bolt->stats.init_time, &t[1], &t[0]);
    return 0;
}

int BoltApplication_debug(struct BoltApplication * bolt, const char * statement)
{
    BoltLog_set_file(stderr);

    struct timespec t[7];

    timespec_get(&t[1], TIME_UTC);    // Checkpoint 1 - right at the start

    BoltApplication_connect(bolt);
    assert(bolt->connection->status == BOLT_CONNECTED);

    BoltApplication_init(bolt);
    assert(bolt->connection->status == BOLT_READY);

    timespec_get(&t[2], TIME_UTC);    // Checkpoint 2 - after handshake and initialisation

    //BoltConnection_load_bookmark(bolt->connection, "tx:1234");
    BoltConnection_load_begin_request(bolt->connection);
    BoltConnection_set_cypher_template(bolt->connection, statement, (int32_t)(strlen(statement)));
    BoltConnection_set_n_cypher_parameters(bolt->connection, 0);
    BoltConnection_load_run_request(bolt->connection);
    bolt_request_t run = BoltConnection_last_request(bolt->connection);
    BoltConnection_load_pull_request(bolt->connection, -1);
    bolt_request_t pull = BoltConnection_last_request(bolt->connection);
    BoltConnection_load_commit_request(bolt->connection);
    bolt_request_t commit = BoltConnection_last_request(bolt->connection);

    BoltConnection_send_b(bolt->connection);

    timespec_get(&t[3], TIME_UTC);    // Checkpoint 3 - after query transmission

    long record_count = 0;

    BoltConnection_fetch_summary_b(bolt->connection, run);

    timespec_get(&t[4], TIME_UTC);    // Checkpoint 4 - receipt of header

    while (BoltConnection_fetch_b(bolt->connection, pull))
    {
        record_count += 1;
    }

    BoltConnection_fetch_summary_b(bolt->connection, commit);

    timespec_get(&t[5], TIME_UTC);    // Checkpoint 5 - receipt of footer

    BoltConnection_close_b(bolt->connection);

    timespec_get(&t[6], TIME_UTC);    // Checkpoint 6 - after close

    ///////////////////////////////////////////////////////////////////

    fprintf(stderr, "query                : %s\n", statement);
    fprintf(stderr, "record count         : %ld\n", record_count);
    fprintf(stderr, "=====================================\n");

    timespec_diff(&t[0], &t[2], &t[1]);
    fprintf(stderr, "initialisation       : %lds %09ldns\n", (long)t[0].tv_sec, t[0].tv_nsec);

    timespec_diff(&t[0], &t[3], &t[2]);
    fprintf(stderr, "query transmission   : %lds %09ldns\n", (long)t[0].tv_sec, t[0].tv_nsec);

    timespec_diff(&t[0], &t[4], &t[3]);
    fprintf(stderr, "query processing     : %lds %09ldns\n", (long)t[0].tv_sec, t[0].tv_nsec);

    timespec_diff(&t[0], &t[5], &t[4]);
    fprintf(stderr, "result processing    : %lds %09ldns\n", (long)t[0].tv_sec, t[0].tv_nsec);

    timespec_diff(&t[0], &t[6], &t[5]);
    fprintf(stderr, "shutdown             : %lds %09ldns\n", (long)t[0].tv_sec, t[0].tv_nsec);

    timespec_diff(&t[0], &t[6], &t[1]);
    fprintf(stderr, "=====================================\n");
    fprintf(stderr, "TOTAL                : %lds %09ldns\n", (long)t[0].tv_sec, t[0].tv_nsec);

    return 0;
}

int BoltApplication_export(struct BoltApplication * bolt, const char * statement)
{
    BoltApplication_connect(bolt);
    assert(bolt->connection->status == BOLT_CONNECTED);

    BoltApplication_init(bolt);
    assert(bolt->connection->status == BOLT_READY);

    BoltConnection_set_cypher_template(bolt->connection, statement, (int32_t)(strlen(statement)));
    BoltConnection_set_n_cypher_parameters(bolt->connection, 0);
    BoltConnection_load_run_request(bolt->connection);
    bolt_request_t run = BoltConnection_last_request(bolt->connection);
    BoltConnection_load_pull_request(bolt->connection, -1);
    bolt_request_t pull = BoltConnection_last_request(bolt->connection);

    BoltConnection_send_b(bolt->connection);

    BoltConnection_fetch_summary_b(bolt->connection, run);
    BoltConnection_dump_field_names(bolt->connection, stdout);

    while (BoltConnection_fetch_b(bolt->connection, pull))
    {
        BoltConnection_dump_data(bolt->connection, stdout);
    }

    BoltConnection_close_b(bolt->connection);

    return 0;
}

void help(const char * argv0)
{
    fprintf(stderr, "%s debug <statement>", argv0);
    fprintf(stderr, "%s export <statement>", argv0);
    exit(EXIT_SUCCESS);
}

int main(int argc, char* argv[])
{
	Bolt_startup();

    struct BoltApplication* bolt = BoltApplication_create();
    if (argc < 2)
    {
        help(argv[0]);
    }
    if (strcmp(argv[1], "debug") == 0)
    {
        if (argc < 3)
        {
            help(argv[0]);
        }
        BoltApplication_debug(bolt, argv[2]);
    }
    else if (strcmp(argv[1], "export") == 0)
    {
        if (argc < 3)
        {
            help(argv[0]);
        }
        BoltApplication_export(bolt, argv[2]);
    }
    else
    {
        help(argv[0]);
    }
    BoltApplication_destroy(bolt);

    if (strcmp(argv[1], "debug") == 0)
    {
        fprintf(stderr, "=====================================\n");
        fprintf(stderr, "current allocation   : %ld bytes\n", BoltMem_current_allocation());
        fprintf(stderr, "peak allocation      : %ld bytes\n", BoltMem_peak_allocation());
        fprintf(stderr, "allocation events    : %lld\n", BoltMem_allocation_events());
    }

	Bolt_shutdown();
	
    return 0;
}
