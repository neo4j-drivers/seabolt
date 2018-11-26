Using Seabolt
*************

CMake support
=============

Our build pipeline generates `CMake` bindings for inclusion into cmake projects.

The first step is creating our `CMakeLists.txt` file. With the help of `CMake` bindings we can use built-in
`find_package` function and ask for `seabolt17` package to be `REQUIRED`.
This ensures that if `seabolt17` is not found, the build process will fail. If you are sure that `CMake` bindings for
`seabolt17` is installed but it is not discovered, follow the instructions provided by `CMake` to explicitly set the
location where `CMake` bindings are installed.

Having been found, you can then add seabolt as a `target_link_library` to your target and provide either
`seabolt17::seabolt-static` or `seabolt17::seabolt-shared` as the dependent library and all your includes and link
libraries will be set accordingly.

CMakeLists.txt
++++++++++++++

.. code-block:: cmake

    cmake_minimum_required(VERSION 3.11 FATAL_ERROR)

    project(seabolt-cmake
            DESCRIPTION "Seabolt Demo"
            LANGUAGES C)

    add_executable(seabolt-cmake main.c)

    find_package(seabolt17 REQUIRED)
    target_link_libraries(seabolt-cmake seabolt17::seabolt-static)

Lifecycle Functions
===================

The first function to be invoked from seabolt should be `Bolt_startup` and the last method should be `Bolt_shutdown`.
These functions ensure that related data structures are allocated/deallocated and pre/post initialisation logic is
carried out.

Below is a code listing with calls to lifecycle functions and a couple of utility macro and function boiler-plates.

main.c
++++++

.. code-block:: c

    #include <stdio.h>
    #include <string.h>
    #include <bolt/bolt.h>

    #define TRY(connection, code) { \
        int error_try = (code); \
        if (error_try != BOLT_SUCCESS) { \
            check_and_print_error(connection, BoltConnection_status(connection), NULL); \
            return error_try; \
        } \
    }

    int32_t check_and_print_error(BoltConnection* connection, BoltStatus* status, const char* error_text)
    {
        int32_t error_code = BoltStatus_get_error(status);
        if (error_code==BOLT_SUCCESS) {
            return BOLT_SUCCESS;
        }

        if (error_code==BOLT_SERVER_FAILURE) {
            char string_buffer[4096];
            if (BoltValue_to_string(BoltConnection_failure(connection), string_buffer, 4096, connection)>4096) {
                string_buffer[4095] = 0;
            }
            fprintf(stderr, "%s: %s", error_text==NULL ? "server failure" : error_text, string_buffer);
        }
        else {
            const char* error_ctx = BoltStatus_get_error_context(status);
            fprintf(stderr, "%s (code: %04X, text: %s, context: %s)\n", error_text==NULL ? "Bolt failure" : error_text,
                    error_code, BoltError_get_string(error_code), error_ctx);
        }
        return error_code;
    }

    int main(int argc, char** argv) {
        Bolt_startup();

        Bolt_shutdown();
    }


Creating a connector instance
=============================

For a connector instance to be created, we need to pass `BoltConnector_create` call an address, an authentication token
and a configuration.

- Address instances can be created using `BoltAddress_create`.
- Authentication token instances can be created using helper functions with names scoped under `BoltAuth`. Currently we
  only provide `BoltAuth_basic` as it is the most widely used. For any other tokens supported by the server, you can create
  `Dictionary` typed `BoltValue` instances that include related `key` and `value` pairs representing the authentication
  token of interest.
- Configuration instances can be created using `BoltConfig_create`. You can tweak config options available using the
  setter and getter functions with names scoped under `BoltConfig`.

.. code-block:: c

    BoltConnector* create_connector()
    {
        BoltAddress* address = BoltAddress_create("localhost", "7687");
        BoltValue* auth_token = BoltAuth_basic("neo4j", "password", NULL);
        BoltConfig* config = BoltConfig_create();
        BoltConfig_set_user_agent(config, "seabolt-cmake/1.7");

        BoltConnector* connector = BoltConnector_create(address, auth_token, config);

        BoltAddress_destroy(address);
        BoltValue_destroy(auth_token);
        BoltConfig_destroy(config);

        return connector;
    }

Acquiring a connection
======================

After connector is initialised, we can ask it to create connection instances for us. Please note that the connector
manages a pool of connections underneath.

We ask for a connection instance via `BoltConnector_acquire` and when successful it returns a valid connection instance
that we can use to execute queries. Whenever an error occurs, it returns `NULL` as result and sets related fields of
`BoltStatus` to provide more information about the underlying error.

You can see the implementation of `check_and_print_error` function above to learn how to get more details about an
error from the `BoltStatus` instance.

.. code-block:: c

    BoltStatus* status = BoltStatus_create();

    BoltConnection* connection = BoltConnector_acquire(connector, BOLT_ACCESS_MODE_READ, status);
    if (connection!=NULL) {
        run_cypher(connection, "UNWIND RANGE(1, $x) AS N RETURN N", "x", 5);
    }
    else {
        check_and_print_error(NULL, status, "unable to acquire connection");
    }

    BoltStatus_destroy(status);

Executing a query
=================

Query execution is formed of a couple of consecutive calls.

First, you'll need to set the cypher statement to execute along with the number of parameters to be passed via
`BoltConnection_set_run_cypher`. You can set each parameter key and get a value instance for it to be populated using
`BoltConnection_set_run_cypher_parameter`.

After setting cypher statements, parameters (if any) and any possible options related with the run message (functions
scoped under `BoltConnection_set_run`), we are ready to queue the actual message into the transmission queue with
`BoltConnection_load_run_request`.

We can get an handle to the latest queued message using `BoltConnection_last_request`.

We explicitly need to pull or discard results that will be generated by the queued run message. We can pull results
by queuing by `BoltConnection_load_pull_request` or discard them by `BoltConnection_load_discard_request`.

Finally, we send all queued messages using `BoltConnection_send`.

.. code-block:: c

    int32_t run_cypher(BoltConnection* connection, const char* cypher, const char* p_name, int64_t p_value)
    {
        TRY(connection, BoltConnection_set_run_cypher(connection, cypher, strlen(cypher), 1));
        BoltValue_format_as_Integer(BoltConnection_set_run_cypher_parameter(connection, 0, p_name, strlen(p_name)), p_value);
        TRY(connection, BoltConnection_load_run_request(connection));
        BoltRequest run = BoltConnection_last_request(connection);

        TRY(connection, BoltConnection_load_pull_request(connection, -1));
        BoltRequest pull_all = BoltConnection_last_request(connection);

        TRY(connection, BoltConnection_send(connection));

        // processing results
        // ..
        // ..
    }

Processing results
==================

Having sent our requests to the server, now it is time to process the response generated.

First of all, we check the status of our run request. We ask for corresponding summary response using
`BoltConnection_fetch_summary` and check whether the server responded with a success message or not with
`BoltConnection_summary_success`. If we find it to be failed, we'll call our utility function named
`check_and_print_error`.

Success message for run message will return us list of field names that'll be returned in the result set. We get that
list with `BoltConnection_field_names` and print each field name seperated with a tab character.

Finished processing the run message, we come to the pull message response. This is typically a loop construct looping
until we receive a summary message. `BoltConnection_fetch` returns `1` for record messages, `0` for summary messages
and `-1` when error occurs. We loop for record messages and retrieve the returned field value list using
`BoltConnection_field_values`. We just loop throught the field value list and print each field value seperated with a
tab character.

After exiting the result set loop, we check the status of the connection and report any failures stored.

.. code-block:: c

    int32_t run_cypher(BoltConnection* connection, const char* cypher, int64_t x)
    {
        // executing a query
        // ..
        // ..

        char string_buffer[4096];
        if (BoltConnection_fetch_summary(connection, run)<0 || !BoltConnection_summary_success(connection)) {
            return check_and_print_error(connection, BoltConnection_status(connection), "cypher execution failed");
        }

        const BoltValue* field_names = BoltConnection_field_names(connection);
        for (int i = 0; i<BoltValue_size(field_names); i++) {
            BoltValue* field_name = BoltList_value(field_names, i);
            if (i>0) {
                printf("\t");
            }
            if (BoltValue_to_string(field_name, string_buffer, 4096, connection)>4096) {
                string_buffer[4095] = 0;
            }
            printf("%-12s", string_buffer);
        }
        printf("\n");

        while (BoltConnection_fetch(connection, pull_all)>0) {
            const BoltValue* field_values = BoltConnection_field_values(connection);
            for (int i = 0; i<BoltValue_size(field_values); i++) {
                BoltValue* field_value = BoltList_value(field_values, i);
                if (i>0) {
                    printf("\t");
                }
                if (BoltValue_to_string(field_value, string_buffer, 4096, connection)>4096) {
                    string_buffer[4095] = 0;
                }
                printf("%s", string_buffer);
            }
            printf("\n");
        }

        return check_and_print_error(connection, BoltConnection_status(connection), "cypher execution failed");
    }

Releasing the connection
========================

When you're done with a connection previously acquired from the connector, you are supposed to release it back to the
connector - and it will be cached in the connection pool and be available for later acquire calls.

.. code-block:: c

    BoltConnector_release(connector, connection);

Destroying the connector
========================

When we're finished with the connector, we need to destroy it to cleanly close all pooled connections and free the
memory used by the connector and its internal structures.

.. code-block:: c

    BoltConnector_destroy(connector);


Complete code listing
=====================

CMakeLists.txt
++++++++++++++

.. code-block:: cmake

    cmake_minimum_required(VERSION 3.11 FATAL_ERROR)

    project(seabolt-cmake
            DESCRIPTION "Seabolt Demo"
            LANGUAGES C)

    add_executable(seabolt-cmake main.c)

    find_package(seabolt17 REQUIRED)
    target_link_libraries(seabolt-cmake seabolt17::seabolt-static)

main.c
++++++

.. code-block:: c

    #include <stdio.h>
    #include <string.h>
    #include <bolt/bolt.h>

    #define TRY(connection, code) { \
        int error_try = (code); \
        if (error_try != BOLT_SUCCESS) { \
            check_and_print_error(connection, BoltConnection_status(connection), NULL); \
            return error_try; \
        } \
    }

    int32_t check_and_print_error(BoltConnection* connection, BoltStatus* status, const char* error_text)
    {
        int32_t error_code = BoltStatus_get_error(status);
        if (error_code==BOLT_SUCCESS) {
            return BOLT_SUCCESS;
        }

        if (error_code==BOLT_SERVER_FAILURE) {
            char string_buffer[4096];
            if (BoltValue_to_string(BoltConnection_failure(connection), string_buffer, 4096, connection)>4096) {
                string_buffer[4095] = 0;
            }
            fprintf(stderr, "%s: %s", error_text==NULL ? "server failure" : error_text, string_buffer);
        }
        else {
            const char* error_ctx = BoltStatus_get_error_context(status);
            fprintf(stderr, "%s (code: %04X, text: %s, context: %s)\n", error_text==NULL ? "Bolt failure" : error_text,
                    error_code, BoltError_get_string(error_code), error_ctx);
        }
        return error_code;
    }

    BoltConnector* create_connector()
    {
        BoltAddress* address = BoltAddress_create("localhost", "7687");
        BoltValue* auth_token = BoltAuth_basic("neo4j", "password", NULL);
        BoltConfig* config = BoltConfig_create();
        BoltConfig_set_user_agent(config, "seabolt-cmake/1.7");

        BoltConnector* connector = BoltConnector_create(address, auth_token, config);

        BoltAddress_destroy(address);
        BoltValue_destroy(auth_token);
        BoltConfig_destroy(config);

        return connector;
    }

    int32_t run_cypher(BoltConnection* connection, const char* cypher, const char* p_name, int64_t p_value)
    {
        TRY(connection, BoltConnection_set_run_cypher(connection, cypher, strlen(cypher), 1));
        BoltValue_format_as_Integer(BoltConnection_set_run_cypher_parameter(connection, 0, p_name, strlen(p_name)), p_value);
        TRY(connection, BoltConnection_load_run_request(connection));
        BoltRequest run = BoltConnection_last_request(connection);

        TRY(connection, BoltConnection_load_pull_request(connection, -1));
        BoltRequest pull_all = BoltConnection_last_request(connection);

        TRY(connection, BoltConnection_send(connection));

        char string_buffer[4096];
        if (BoltConnection_fetch_summary(connection, run)<0 || !BoltConnection_summary_success(connection)) {
            return check_and_print_error(connection, BoltConnection_status(connection), "cypher execution failed");
        }

        const BoltValue* field_names = BoltConnection_field_names(connection);
        for (int i = 0; i<BoltValue_size(field_names); i++) {
            BoltValue* field_name = BoltList_value(field_names, i);
            if (i>0) {
                printf("\t");
            }
            if (BoltValue_to_string(field_name, string_buffer, 4096, connection)>4096) {
                string_buffer[4095] = 0;
            }
            printf("%-12s", string_buffer);
        }
        printf("\n");

        while (BoltConnection_fetch(connection, pull_all)>0) {
            const BoltValue* field_values = BoltConnection_field_values(connection);
            for (int i = 0; i<BoltValue_size(field_values); i++) {
                BoltValue* field_value = BoltList_value(field_values, i);
                if (i>0) {
                    printf("\t");
                }
                if (BoltValue_to_string(field_value, string_buffer, 4096, connection)>4096) {
                    string_buffer[4095] = 0;
                }
                printf("%s", string_buffer);
            }
            printf("\n");
        }

        return check_and_print_error(connection, BoltConnection_status(connection), "cypher execution failed");
    }

    int main(int argc, char** argv) {
        Bolt_startup();

        BoltConnector *connector = create_connector();

        BoltStatus* status = BoltStatus_create();

        BoltConnection* connection = BoltConnector_acquire(connector, BOLT_ACCESS_MODE_READ, status);
        if (connection!=NULL) {
            run_cypher(connection, "UNWIND RANGE(1, $x) AS N RETURN N", "x", 5);
        }
        else {
            check_and_print_error(NULL, status, "unable to acquire connection");
        }

        BoltStatus_destroy(status);

        BoltConnector_release(connector, connection);

        BoltConnector_destroy(connector);

        Bolt_shutdown();
    }
