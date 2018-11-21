==========
Connection
==========

Connections provide the core means of communication to a Application server.

--------------
BoltConnection
--------------

.. doxygentypedef:: BoltConnection

.. doxygenfunction:: BoltConnection_create

.. doxygenfunction:: BoltConnection_destroy

.. doxygenfunction:: BoltConnection_open

.. doxygenfunction:: BoltConnection_close

.. doxygenfunction:: BoltConnection_init

.. doxygenfunction:: BoltConnection_send

.. doxygenfunction:: BoltConnection_fetch

.. doxygenfunction:: BoltConnection_fetch_summary

.. doxygenfunction:: BoltConnection_clear_begin

.. doxygenfunction:: BoltConnection_set_begin_bookmarks

.. doxygenfunction:: BoltConnection_set_begin_tx_timeout

.. doxygenfunction:: BoltConnection_set_begin_tx_metadata

.. doxygenfunction:: BoltConnection_load_begin_request

.. doxygenfunction:: BoltConnection_load_commit_request

.. doxygenfunction:: BoltConnection_load_rollback_request

.. doxygenfunction:: BoltConnection_clear_run

.. doxygenfunction:: BoltConnection_set_run_bookmarks

.. doxygenfunction:: BoltConnection_set_run_tx_timeout

.. doxygenfunction:: BoltConnection_set_run_tx_metadata

.. doxygenfunction:: BoltConnection_set_run_cypher

.. doxygenfunction:: BoltConnection_set_run_cypher_parameter

.. doxygenfunction:: BoltConnection_load_run_request

.. doxygenfunction:: BoltConnection_load_pull_request

.. doxygenfunction:: BoltConnection_load_discard_request

.. doxygenfunction:: BoltConnection_load_reset_request

.. doxygenfunction:: BoltConnection_last_request

.. doxygenfunction:: BoltConnection_server

.. doxygenfunction:: BoltConnection_id

.. doxygenfunction:: BoltConnection_address

.. doxygenfunction:: BoltConnection_remote_endpoint

.. doxygenfunction:: BoltConnection_local_endpoint

.. doxygenfunction:: BoltConnection_last_bookmark

.. doxygenfunction:: BoltConnection_summary_success

.. doxygenfunction:: BoltConnection_failure

.. doxygenfunction:: BoltConnection_field_names

.. doxygenfunction:: BoltConnection_field_values

.. doxygenfunction:: BoltConnection_metadata

.. doxygenfunction:: BoltConnection_status

----------
BoltStatus
----------

.. doxygentypedef:: BoltStatus

.. doxygenfunction:: BoltStatus_create

.. doxygenfunction:: BoltStatus_destroy

.. doxygenfunction:: BoltStatus_get_state

.. doxygenfunction:: BoltStatus_get_error

.. doxygenfunction:: BoltStatus_get_error_context
