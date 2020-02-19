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


#include <memory.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <inttypes.h>

#include "bolt/bolt.h"
#include "bolt/time.h"

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
#include <Availability.h>

#ifdef __MAC_OS_X_VERSION_MIN_REQUIRED
    #if __MAC_OS_X_VERSION_MIN_REQUIRED < 10153
        #define TIME_UTC 0

        int timespec_get(struct timespec *ts, int type)
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
#endif
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
    BoltAccessMode access_mode;
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

void log_to_stderr(void* state, const char* message)
{
    UNUSED(state);
    fprintf(stderr, "%s\n", message);
}

struct BoltLog* create_logger(int enabled)
{
    struct BoltLog* log = BoltLog_create(0);
    BoltLog_set_debug_func(log, enabled ? log_to_stderr : NULL);
    BoltLog_set_warning_func(log, enabled ? log_to_stderr : NULL);
    BoltLog_set_info_func(log, enabled ? log_to_stderr : NULL);
    BoltLog_set_error_func(log, log_to_stderr);
    return log;
}

void app_help()
{
    fprintf(stderr, "seabolt help\n");
    fprintf(stderr, "seabolt debug <cypher>\n");
    fprintf(stderr, "seabolt perf <warmup_times> <actual_times> <cypher>\n");
    fprintf(stderr, "seabolt run <cypher>\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "supported environment variables\n");
    fprintf(stderr, "  %-16s: 0 for Direct Driver, 1 for Routing (Default: 0)\n", "BOLT_ROUTING");
    fprintf(stderr, "  %-16s: WRITE for write mode, READ for read mode (Default: WRITE)\n", "BOLT_ACCESS_MODE");
    fprintf(stderr, "  %-16s: 0 for non-encrypted communication, 1 for TLS (Default: 1)\n", "BOLT_SECURE");
    fprintf(stderr, "  %-16s: Hostname to connect to (Default: localhost)\n", "BOLT_HOST");
    fprintf(stderr, "  %-16s: Port to connect to (Default: 7687)\n", "BOLT_PORT");
    fprintf(stderr, "  %-16s: Username, set to empty to disable authentication token (Default: neo4j)\n", "BOLT_USER");
    fprintf(stderr, "  %-16s: Password\n", "BOLT_PASSWORD");
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

    // Verify environment variables
    int valid_config = strcmp(BOLT_CONFIG_ROUTING, "")!=0 && strcmp(BOLT_CONFIG_ACCESS_MODE, "")!=0
            && strcmp(BOLT_CONFIG_SECURE, "")!=0 && strcmp(BOLT_CONFIG_HOST, "")!=0 && strcmp(BOLT_CONFIG_PORT, "")!=0;
    if (!valid_config) {
        app_help();
        exit(EXIT_FAILURE);
    }

    // Verify password is provided when user is set
    if (strcmp(BOLT_CONFIG_USER, "")!=0 && (BOLT_CONFIG_PASSWORD==NULL || strcmp(BOLT_CONFIG_PASSWORD, "")==0)) {
        app_help();
        exit(EXIT_FAILURE);
    }

    struct Application* app = malloc(sizeof(struct Application));

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

    BoltLog* log = create_logger(app->command==CMD_DEBUG);
    BoltConfig* config = BoltConfig_create();
    BoltConfig_set_scheme(config, (strcmp(BOLT_CONFIG_ROUTING, "1")==0) ? BOLT_SCHEME_NEO4J : BOLT_SCHEME_DIRECT);
    BoltConfig_set_transport(config,
            (strcmp(BOLT_CONFIG_SECURE, "1")==0) ? BOLT_TRANSPORT_ENCRYPTED : BOLT_TRANSPORT_PLAINTEXT);
    BoltConfig_set_user_agent(config, "seabolt/" SEABOLT_VERSION);
    BoltConfig_set_max_pool_size(config, 10);
    BoltConfig_set_log(config, log);

    struct BoltValue* auth_token = NULL;
    if (strcmp(BOLT_CONFIG_USER, "")!=0) {
        auth_token = BoltAuth_basic(BOLT_CONFIG_USER, BOLT_CONFIG_PASSWORD, NULL);
    }
    else {
        auth_token = BoltAuth_none();
    }

    struct BoltAddress* address = BoltAddress_create((char*) BOLT_CONFIG_HOST, (char*) BOLT_CONFIG_PORT);

    app->connector = BoltConnector_create(address, auth_token, config);

    BoltValue_destroy(auth_token);
    BoltAddress_destroy(address);
    BoltLog_destroy(log);
    BoltConfig_destroy(config);

    return app;
}

void app_destroy(struct Application* app)
{
    BoltConnector_destroy(app->connector);
    free(app);
}

void app_connect(struct Application* app)
{
    struct timespec t[2];
    BoltTime_get_time(&t[0]);
    BoltStatus* status = BoltStatus_create();
    BoltConnection* connection = BoltConnector_acquire(app->connector, app->access_mode, status);
    if (connection==NULL) {
        fprintf(stderr, "FATAL: Failed to connect\n");
        app_destroy(app);
        exit(EXIT_FAILURE);
    }
    BoltStatus_destroy(status);
    app->connection = connection;
    BoltTime_get_time(&t[1]);
    timespec_diff(&app->stats.connect_time, &t[1], &t[0]);
}

int app_debug(struct Application* app, const char* cypher)
{
    struct timespec t[7];

    BoltTime_get_time(&t[1]);    // Checkpoint 1 - right at the start

    app_connect(app);

    BoltTime_get_time(&t[2]);    // Checkpoint 2 - after handshake and initialisation

    //BoltConnection_load_bookmark(bolt->connection, "tx:1234");
    BoltConnection_load_begin_request(app->connection);
    BoltConnection_set_run_cypher(app->connection, cypher, strlen(cypher), 0);
    BoltConnection_load_run_request(app->connection);
    BoltRequest run = BoltConnection_last_request(app->connection);
    BoltConnection_load_pull_request(app->connection, -1);
    BoltRequest pull = BoltConnection_last_request(app->connection);
    BoltConnection_load_commit_request(app->connection);
    BoltRequest commit = BoltConnection_last_request(app->connection);

    BoltConnection_send(app->connection);

    BoltTime_get_time(&t[3]);    // Checkpoint 3 - after query transmission

    long record_count = 0;

    BoltConnection_fetch_summary(app->connection, run);

    BoltTime_get_time(&t[4]);    // Checkpoint 4 - receipt of header

    while (BoltConnection_fetch(app->connection, pull)) {
        record_count += 1;
    }

    BoltConnection_fetch_summary(app->connection, commit);

    BoltTime_get_time(&t[5]);    // Checkpoint 5 - receipt of footer

    BoltConnector_release(app->connector, app->connection);

    BoltTime_get_time(&t[6]);    // Checkpoint 6 - after close

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
    BoltRequest run = BoltConnection_last_request(app->connection);
    BoltConnection_load_pull_request(app->connection, -1);
    BoltRequest pull = BoltConnection_last_request(app->connection);

    BoltConnection_send(app->connection);

    BoltConnection_fetch_summary(app->connection, run);
    char string_buffer[4096];
    if (app->with_header) {
        const struct BoltValue* fields = BoltConnection_field_names(app->connection);
        for (int i = 0; i<BoltValue_size(fields); i++) {
            if (i>0) {
                putc('\t', stdout);
            }
            if (BoltValue_to_string(BoltList_value(fields, i), string_buffer, 4096, app->connection)>4096) {
                string_buffer[4095] = 0;
            }
            fprintf(stdout, "%s", string_buffer);
        }
        putc('\n', stdout);
    }

    while (BoltConnection_fetch(app->connection, pull)) {
        const struct BoltValue* field_values = BoltConnection_field_values(app->connection);
        for (int i = 0; i<BoltValue_size(field_values); i++) {
            struct BoltValue* value = BoltList_value(field_values, i);
            if (i>0) {
                putc('\t', stdout);
            }
            if (BoltValue_to_string(value, string_buffer, 4096, app->connection)>4096) {
                string_buffer[4095] = 0;
            }
            fprintf(stdout, "%s", string_buffer);
        }
        putc('\n', stdout);
    }

    BoltConnector_release(app->connector, app->connection);

    return 0;
}

long run_fetch(const struct Application* app, const char* cypher)
{
    long record_count = 0;
    BoltConnection_load_begin_request(app->connection);
    BoltConnection_set_run_cypher(app->connection, cypher, strlen(cypher), 0);
    BoltConnection_load_run_request(app->connection);
    BoltRequest run = BoltConnection_last_request(app->connection);
    BoltConnection_load_pull_request(app->connection, -1);
    BoltRequest pull = BoltConnection_last_request(app->connection);
    BoltConnection_load_commit_request(app->connection);
    BoltRequest commit = BoltConnection_last_request(app->connection);

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

    BoltTime_get_time(&t[1]);    // Checkpoint 1 - before run
    long record_count = 0;
    for (int i = 0; i<actual_times; i++) {
        record_count += run_fetch(app, cypher);
    }
    BoltTime_get_time(&t[2]);    // Checkpoint 2 - after run

    BoltConnector_release(app->connector, app->connection);

    ///////////////////////////////////////////////////////////////////

    fprintf(stderr, "query                : %s\n", cypher);
    fprintf(stderr, "record count         : %ld\n", record_count);

    timespec_diff(&t[0], &t[2], &t[1]);
    fprintf(stderr, "=====================================\n");
    fprintf(stderr, "TOTAL TIME           : %lds %09ldns\n", (long) t[0].tv_sec, t[0].tv_nsec);

    return 0;
}

int main(int argc, char* argv[])
{
    Bolt_startup();

    struct Application* app = app_create(argc, argv);

    switch (app->command) {
    case CMD_NONE:
    case CMD_HELP:
        app_help();
        exit(EXIT_SUCCESS);
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
        fprintf(stderr, "current allocation   : %" PRIu64 " bytes\n", (int64_t) BoltStat_memory_allocation_current());
        fprintf(stderr, "peak allocation      : %" PRId64 " bytes\n", (int64_t) BoltStat_memory_allocation_peak());
        fprintf(stderr, "allocation events    : %" PRId64 "\n", BoltStat_memory_allocation_events());
        fprintf(stderr, "=====================================\n");
    }

    if (BoltStat_memory_allocation_current()==0) {
        return EXIT_SUCCESS;
    }
    else {
        fprintf(stderr, "MEMORY LEAK!\n");
        return EXIT_FAILURE;
    }
}
