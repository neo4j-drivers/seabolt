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


#include <memory.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <inttypes.h>

#include "auth.h"
#include "connector.h"
#include "connections.h"
#include "lifecycle.h"
#include "mem.h"
#include "values.h"
#include "platform.h"
#include "logging.h"
#include "protocol.h"
#include "string-builder.h"

#ifdef WIN32

#include <winsock2.h>

#endif // WIN32

#if defined(_WIN32) && _MSC_VER
#pragma  warning(disable:4204)
#endif

#define UNUSED(x) (void)(x)

#ifdef __APPLE__
#include <mach/clock.h>
#include <mach/mach.h>

#define TIME_UTC 0

void timespec_get(struct timespec *ts, int type)
{
    UNUSED(type);
    clock_serv_t cclock;
    mach_timespec_t mts;
    host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &cclock);
    clock_get_time(cclock, &mts);
    mach_port_deallocate(mach_task_self(), cclock);
    ts->tv_sec = mts.tv_sec;
    ts->tv_nsec = mts.tv_nsec;
}
#endif

enum Command {
    CMD_NONE,
    CMD_HELP,
    CMD_DEBUG,
    CMD_PERF,
    CMD_RUN,
};

struct Application {
    struct BoltConnector* connector;
    struct BoltConnection* connection;
    enum BoltAccessMode access_mode;
    struct {
        struct timespec connect_time;
        struct timespec init_time;
    } stats;
    int with_allocation_report;
    int with_header;
    enum Command command;
    int first_arg_index;
    int argc;
    char** argv;
};

const char* getenv_or_default(const char* name, const char* default_value)
{
    const char* value = getenv(name);
    return (value==NULL) ? default_value : value;
}

void timespec_diff(struct timespec* t, struct timespec* t0, struct timespec* t1)
{
    t->tv_sec = t0->tv_sec-t1->tv_sec;
    t->tv_nsec = t0->tv_nsec-t1->tv_nsec;
    while (t->tv_nsec>=1000000000) {
        t->tv_sec += 1;
        t->tv_nsec -= 1000000000;
    }
    while (t->tv_nsec<0) {
        t->tv_sec -= 1;
        t->tv_nsec += 1000000000;
    }
}

void log_to_stderr(int state, const char* message)
{
    UNUSED(state);
    fprintf(stderr, "%s\n", message);
}

struct BoltLog* create_logger()
{
    struct BoltLog* log = BoltLog_create();
    log->state = 0;
    log->debug_enabled = 0;
    log->info_enabled = 0;
    log->warning_enabled = 0;
    log->error_enabled = 0;
    log->debug_logger = log_to_stderr;
    log->info_logger = log_to_stderr;
    log->warning_logger = log_to_stderr;
    log->error_logger = log_to_stderr;
    return log;
}

struct Application* app_create(int argc, char** argv)
{
    const char* BOLT_CONFIG_ROUTING = getenv_or_default("BOLT_ROUTING", "0");
    const char* BOLT_CONFIG_ACCESS_MODE = getenv_or_default("BOLT_ACCESS_MODE", "WRITE");
    const char* BOLT_CONFIG_SECURE = getenv_or_default("BOLT_SECURE", "1");
    const char* BOLT_CONFIG_HOST = getenv_or_default("BOLT_HOST", "localhost");
    const char* BOLT_CONFIG_PORT = getenv_or_default("BOLT_PORT", "7687");
    const char* BOLT_CONFIG_USER = getenv_or_default("BOLT_USER", "neo4j");
    const char* BOLT_CONFIG_PASSWORD = getenv("BOLT_PASSWORD");

    struct Application* app = BoltMem_allocate(sizeof(struct Application));

    struct BoltConfig config;
    config.mode = (strcmp(BOLT_CONFIG_ROUTING, "1")==0 ? BOLT_ROUTING : BOLT_DIRECT);
    config.transport = (strcmp(BOLT_CONFIG_SECURE, "1")==0) ? BOLT_SECURE_SOCKET : BOLT_SOCKET;
    config.routing_context = NULL;
    config.user_agent = "seabolt/1.0.0a";
    config.max_pool_size = 10;
    config.log = create_logger();
    config.address_resolver = NULL;
    config.trust = NULL;
    config.sock_opts = NULL;

    struct BoltValue* auth_token = BoltAuth_basic(BOLT_CONFIG_USER, BOLT_CONFIG_PASSWORD, NULL);

    app->connector = BoltConnector_create(&BoltAddress_of((char*) BOLT_CONFIG_HOST, (char*) BOLT_CONFIG_PORT),
            auth_token, &config);

    BoltValue_destroy(auth_token);
    BoltLog_destroy(config.log);

    app->access_mode = (strcmp(BOLT_CONFIG_ACCESS_MODE, "WRITE")==0 ? BOLT_ACCESS_MODE_WRITE : BOLT_ACCESS_MODE_READ);
    app->with_allocation_report = 0;
    app->with_header = 0;
    app->command = CMD_NONE;
    app->first_arg_index = -1;
    app->argv = argv;
    app->argc = argc;

    for (int i = 1; i<argc; i++) {
        const char* arg = argv[i];
        if (strlen(arg)>=1 && arg[0]=='-') {
            // option
            if (strcmp(arg, "-a")==0) {
                app->with_allocation_report = 1;
            }
            else if (strcmp(arg, "-h")==0) {
                app->with_header = 1;
            }
            else {
                fprintf(stderr, "Unknown option %s\n", arg);
                exit(EXIT_FAILURE);
            }
        }
        else if (app->command==CMD_NONE) {
            // command
            if (strcmp(arg, "help")==0) {
                app->command = CMD_HELP;
            }
            else if (strcmp(arg, "debug")==0) {
                app->command = CMD_DEBUG;
            }
            else if (strcmp(arg, "perf")==0) {
                app->command = CMD_PERF;
            }
            else if (strcmp(arg, "run")==0) {
                app->command = CMD_RUN;
            }
            else {
                fprintf(stderr, "Unknown command %s\n", arg);
                exit(EXIT_FAILURE);
            }
        }
        else {
            // argument
            app->first_arg_index = i;
            break;
        }
    }

    return app;
}

void app_destroy(struct Application* app)
{
    BoltConnector_destroy(app->connector);
    BoltMem_deallocate(app, sizeof(struct Application));
}

void app_connect(struct Application* app)
{
    struct timespec t[2];
    BoltUtil_get_time(&t[0]);
    struct BoltConnectionResult result = BoltConnector_acquire(app->connector, app->access_mode);
    if (result.connection==NULL) {
        fprintf(stderr, "FATAL: Failed to connect\n");
        app_destroy(app);
        exit(EXIT_FAILURE);
    }
    app->connection = result.connection;
    BoltUtil_get_time(&t[1]);
    timespec_diff(&app->stats.connect_time, &t[1], &t[0]);
}

int app_debug(struct Application* app, const char* cypher)
{
    struct timespec t[7];

    BoltUtil_get_time(&t[1]);    // Checkpoint 1 - right at the start

    app->connector->config->log->debug_enabled = 1;
    app->connector->config->log->info_enabled = 1;
    app->connector->config->log->warning_enabled = 1;
    app->connector->config->log->error_enabled = 1;

    app_connect(app);

    BoltUtil_get_time(&t[2]);    // Checkpoint 2 - after handshake and initialisation

    //BoltConnection_load_bookmark(bolt->connection, "tx:1234");
    BoltConnection_load_begin_request(app->connection);
    BoltConnection_set_run_cypher(app->connection, cypher, strlen(cypher), 0);
    BoltConnection_load_run_request(app->connection);
    bolt_request run = BoltConnection_last_request(app->connection);
    BoltConnection_load_pull_request(app->connection, -1);
    bolt_request pull = BoltConnection_last_request(app->connection);
    BoltConnection_load_commit_request(app->connection);
    bolt_request commit = BoltConnection_last_request(app->connection);

    BoltConnection_send(app->connection);

    BoltUtil_get_time(&t[3]);    // Checkpoint 3 - after query transmission

    long record_count = 0;

    BoltConnection_fetch_summary(app->connection, run);

    BoltUtil_get_time(&t[4]);    // Checkpoint 4 - receipt of header

    while (BoltConnection_fetch(app->connection, pull)) {
        record_count += 1;
    }

    BoltConnection_fetch_summary(app->connection, commit);

    BoltUtil_get_time(&t[5]);    // Checkpoint 5 - receipt of footer

    BoltConnector_release(app->connector, app->connection);

    BoltUtil_get_time(&t[6]);    // Checkpoint 6 - after close

    ///////////////////////////////////////////////////////////////////

    fprintf(stderr, "query                : %s\n", cypher);
    fprintf(stderr, "record count         : %ld\n", record_count);
    fprintf(stderr, "=====================================\n");

    timespec_diff(&t[0], &t[2], &t[1]);
    fprintf(stderr, "initialisation       : %lds %09ldns\n", (long) t[0].tv_sec, t[0].tv_nsec);

    timespec_diff(&t[0], &t[3], &t[2]);
    fprintf(stderr, "query transmission   : %lds %09ldns\n", (long) t[0].tv_sec, t[0].tv_nsec);

    timespec_diff(&t[0], &t[4], &t[3]);
    fprintf(stderr, "query processing     : %lds %09ldns\n", (long) t[0].tv_sec, t[0].tv_nsec);

    timespec_diff(&t[0], &t[5], &t[4]);
    fprintf(stderr, "result processing    : %lds %09ldns\n", (long) t[0].tv_sec, t[0].tv_nsec);

    timespec_diff(&t[0], &t[6], &t[5]);
    fprintf(stderr, "shutdown             : %lds %09ldns\n", (long) t[0].tv_sec, t[0].tv_nsec);

    timespec_diff(&t[0], &t[6], &t[1]);
    fprintf(stderr, "=====================================\n");
    fprintf(stderr, "TOTAL                : %lds %09ldns\n", (long) t[0].tv_sec, t[0].tv_nsec);

    BoltConnector_release(app->connector, app->connection);

    return 0;
}

int app_run(struct Application* app, const char* cypher)
{
    app_connect(app);

    BoltConnection_set_run_cypher(app->connection, cypher, strlen(cypher), 0);
    BoltConnection_load_run_request(app->connection);
    bolt_request run = BoltConnection_last_request(app->connection);
    BoltConnection_load_pull_request(app->connection, -1);
    bolt_request pull = BoltConnection_last_request(app->connection);

    BoltConnection_send(app->connection);

    BoltConnection_fetch_summary(app->connection, run);
    if (app->with_header) {
        const struct BoltValue* fields = BoltConnection_field_names(app->connection);
        for (int i = 0; i<fields->size; i++) {
            if (i>0) {
                putc('\t', stdout);
            }
            struct StringBuilder* builder = StringBuilder_create();
            BoltValue_write(builder, BoltList_value(fields, i), app->connection->protocol->structure_name);
            fprintf(stdout, "%s", StringBuilder_get_string(builder));
            StringBuilder_destroy(builder);
        }
        putc('\n', stdout);
    }

    while (BoltConnection_fetch(app->connection, pull)) {
        const struct BoltValue* field_values = BoltConnection_field_values(app->connection);
        for (int i = 0; i<field_values->size; i++) {
            struct BoltValue* value = BoltList_value(field_values, i);
            if (i>0) {
                putc('\t', stdout);
            }
            struct StringBuilder* builder = StringBuilder_create();
            BoltValue_write(builder, value, app->connection->protocol->structure_name);
            fprintf(stdout, "%s", StringBuilder_get_string(builder));
            StringBuilder_destroy(builder);
        }
        putc('\n', stdout);
    }

    BoltConnector_release(app->connector, app->connection);

    return 0;
}

long run_fetch(const struct Application* app, const char* cypher)
{
    long record_count = 0;
    //BoltConnection_load_bookmark(bolt->connection, "tx:1234");
    BoltConnection_load_begin_request(app->connection);
    BoltConnection_set_run_cypher(app->connection, cypher, strlen(cypher), 0);
    BoltConnection_load_run_request(app->connection);
    bolt_request run = BoltConnection_last_request(app->connection);
    BoltConnection_load_pull_request(app->connection, -1);
    bolt_request pull = BoltConnection_last_request(app->connection);
    BoltConnection_load_commit_request(app->connection);
    bolt_request commit = BoltConnection_last_request(app->connection);

    BoltConnection_send(app->connection);

    BoltConnection_fetch_summary(app->connection, run);

    while (BoltConnection_fetch(app->connection, pull)) {
        record_count += 1;
    }

    BoltConnection_fetch_summary(app->connection, commit);
    return record_count;
}

int app_perf(struct Application* app, long warmup_times, long actual_times, const char* cypher)
{
    struct timespec t[3];

    app_connect(app);

    for (int i = 0; i<warmup_times; i++) {
        run_fetch(app, cypher);
    }

    BoltUtil_get_time(&t[1]);    // Checkpoint 1 - before run
    long record_count = 0;
    for (int i = 0; i<actual_times; i++) {
        record_count += run_fetch(app, cypher);
    }
    BoltUtil_get_time(&t[2]);    // Checkpoint 2 - after run

    BoltConnector_release(app->connector, app->connection);

    ///////////////////////////////////////////////////////////////////

    fprintf(stderr, "query                : %s\n", cypher);
    fprintf(stderr, "record count         : %ld\n", record_count);

    timespec_diff(&t[0], &t[2], &t[1]);
    fprintf(stderr, "=====================================\n");
    fprintf(stderr, "TOTAL TIME           : %lds %09ldns\n", (long) t[0].tv_sec, t[0].tv_nsec);

    return 0;
}

void app_help()
{
    fprintf(stderr, "seabolt help\n");
    fprintf(stderr, "seabolt debug <cypher>\n");
    fprintf(stderr, "seabolt perf <warmup_times> <actual_times> <cypher>\n");
    fprintf(stderr, "seabolt run <cypher>\n");
    exit(EXIT_SUCCESS);
}

int main(int argc, char* argv[])
{
    Bolt_startup();

    struct Application* app = app_create(argc, argv);

    switch (app->command) {
    case CMD_NONE:
    case CMD_HELP:
        app_help();
        break;
    case CMD_DEBUG:
        app_debug(app, argv[app->first_arg_index]);
        break;
    case CMD_PERF: {
        char* end;
        app_perf(app, strtol(argv[app->first_arg_index], &end, 10),      // warmup_times
                strtol(argv[app->first_arg_index+1], &end, 10),  // actual_times
                argv[app->first_arg_index+2]);                   // cypher
        break;
    }
    case CMD_RUN:
        app_run(app, argv[app->first_arg_index]);
        break;
    }
    int with_allocation_report = app->with_allocation_report;
    app_destroy(app);

    Bolt_shutdown();

    if (with_allocation_report) {
        fprintf(stderr, "=====================================\n");
        fprintf(stderr, "current allocation   : %" PRIu64 " bytes\n", (int64_t) BoltMem_current_allocation());
        fprintf(stderr, "peak allocation      : %" PRId64 " bytes\n", (int64_t) BoltMem_peak_allocation());
        fprintf(stderr, "allocation events    : %" PRId64 "\n", BoltMem_allocation_events());
        fprintf(stderr, "=====================================\n");
    }

    if (BoltMem_current_allocation()==0) {
        return EXIT_SUCCESS;
    }
    else {
        fprintf(stderr, "MEMORY LEAK!\n");
        return EXIT_FAILURE;
    }
}
