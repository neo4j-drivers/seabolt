Seabolt API
===========

Bolt
----

.. doxygenfunction:: Bolt_startup

.. doxygenfunction:: Bolt_shutdown


BoltAccessMode
--------------

.. doxygentypedef:: BoltAccessMode

.. doxygendefine:: BOLT_ACCESS_MODE_WRITE

.. doxygendefine:: BOLT_ACCESS_MODE_READ


BoltAddress
-----------

.. doxygentypedef:: BoltAddress

.. doxygenfunction:: BoltAddress_create

.. doxygenfunction:: BoltAddress_destroy

.. doxygenfunction:: BoltAddress_host

.. doxygenfunction:: BoltAddress_port


BoltAddressSet
--------------

.. doxygentypedef:: BoltAddressSet

.. doxygenfunction:: BoltAddressSet_add


BoltAddressResolver
-------------------

.. doxygentypedef:: address_resolver_func

.. doxygentypedef:: BoltAddressResolver

.. doxygenfunction:: BoltAddressResolver_create

.. doxygenfunction:: BoltAddressResolver_destroy


BoltAuth
--------

Authentication functions provide an easy way to generate authentication tokens that are ready to be sent over to the
server.

Basic
^^^^^

.. doxygenfunction:: BoltAuth_basic


BoltConfig
----------

.. doxygentypedef:: BoltConfig

.. doxygenfunction:: BoltConfig_create

.. doxygenfunction:: BoltConfig_destroy

.. doxygenfunction:: BoltConfig_get_mode

.. doxygenfunction:: BoltConfig_set_mode

.. doxygenfunction:: BoltConfig_get_transport

.. doxygenfunction:: BoltConfig_set_transport

.. doxygenfunction:: BoltConfig_get_trust

.. doxygenfunction:: BoltConfig_set_trust

.. doxygenfunction:: BoltConfig_get_user_agent

.. doxygenfunction:: BoltConfig_set_user_agent

.. doxygenfunction:: BoltConfig_get_routing_context

.. doxygenfunction:: BoltConfig_set_routing_context

.. doxygenfunction:: BoltConfig_get_address_resolver

.. doxygenfunction:: BoltConfig_set_address_resolver

.. doxygenfunction:: BoltConfig_get_log

.. doxygenfunction:: BoltConfig_set_log

.. doxygenfunction:: BoltConfig_get_max_pool_size

.. doxygenfunction:: BoltConfig_set_max_pool_size

.. doxygenfunction:: BoltConfig_get_max_connection_life_time

.. doxygenfunction:: BoltConfig_set_max_connection_life_time

.. doxygenfunction:: BoltConfig_get_max_connection_acquisition_time

.. doxygenfunction:: BoltConfig_set_max_connection_acquisition_time

.. doxygenfunction:: BoltConfig_get_socket_options

.. doxygenfunction:: BoltConfig_set_socket_options


BoltConnection
--------------

.. doxygentypedef:: BoltConnection

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


BoltConnectionState
-------------------

.. doxygentypedef:: BoltConnectionState

.. doxygendefine:: BOLT_CONNECTION_STATE_DISCONNECTED

.. doxygendefine:: BOLT_CONNECTION_STATE_CONNECTED

.. doxygendefine:: BOLT_CONNECTION_STATE_READY

.. doxygendefine:: BOLT_CONNECTION_STATE_FAILED

.. doxygendefine:: BOLT_CONNECTION_STATE_DEFUNCT


BoltConnector
-------------

.. doxygentypedef:: BoltConnector

.. doxygenfunction:: BoltConnector_create

.. doxygenfunction:: BoltConnector_destroy

.. doxygenfunction:: BoltConnector_acquire

.. doxygenfunction:: BoltConnector_release


BoltError
---------

Codes
^^^^^

.. doxygendefine:: BOLT_SUCCESS
.. doxygendefine:: BOLT_UNKNOWN_ERROR
.. doxygendefine:: BOLT_UNSUPPORTED
.. doxygendefine:: BOLT_INTERRUPTED
.. doxygendefine:: BOLT_CONNECTION_RESET
.. doxygendefine:: BOLT_NO_VALID_ADDRESS
.. doxygendefine:: BOLT_TIMED_OUT
.. doxygendefine:: BOLT_PERMISSION_DENIED
.. doxygendefine:: BOLT_OUT_OF_FILES
.. doxygendefine:: BOLT_OUT_OF_MEMORY
.. doxygendefine:: BOLT_OUT_OF_PORTS
.. doxygendefine:: BOLT_CONNECTION_REFUSED
.. doxygendefine:: BOLT_NETWORK_UNREACHABLE
.. doxygendefine:: BOLT_TLS_ERROR
.. doxygendefine:: BOLT_END_OF_TRANSMISSION
.. doxygendefine:: BOLT_SERVER_FAILURE
.. doxygendefine:: BOLT_TRANSPORT_UNSUPPORTED
.. doxygendefine:: BOLT_PROTOCOL_VIOLATION
.. doxygendefine:: BOLT_PROTOCOL_UNSUPPORTED_TYPE
.. doxygendefine:: BOLT_PROTOCOL_NOT_IMPLEMENTED_TYPE
.. doxygendefine:: BOLT_PROTOCOL_UNEXPECTED_MARKER
.. doxygendefine:: BOLT_PROTOCOL_UNSUPPORTED
.. doxygendefine:: BOLT_POOL_FULL
.. doxygendefine:: BOLT_POOL_ACQUISITION_TIMED_OUT
.. doxygendefine:: BOLT_ADDRESS_NOT_RESOLVED
.. doxygendefine:: BOLT_ROUTING_UNABLE_TO_RETRIEVE_ROUTING_TABLE
.. doxygendefine:: BOLT_ROUTING_NO_SERVERS_TO_SELECT
.. doxygendefine:: BOLT_ROUTING_UNABLE_TO_CONSTRUCT_POOL_FOR_SERVER
.. doxygendefine:: BOLT_ROUTING_UNABLE_TO_REFRESH_ROUTING_TABLE
.. doxygendefine:: BOLT_ROUTING_UNEXPECTED_DISCOVERY_RESPONSE
.. doxygendefine:: BOLT_CONNECTION_HAS_MORE_INFO
.. doxygendefine:: BOLT_STATUS_SET

Descriptions
^^^^^^^^^^^^

.. doxygenfunction:: BoltError_get_string


BoltLog
-------

.. doxygentypedef:: log_func

.. doxygentypedef:: BoltLog

.. doxygenfunction:: BoltLog_create

.. doxygenfunction:: BoltLog_destroy

.. doxygenfunction:: BoltLog_set_error_func

.. doxygenfunction:: BoltLog_set_warning_func

.. doxygenfunction:: BoltLog_set_info_func

.. doxygenfunction:: BoltLog_set_debug_func


BoltMode
--------

.. doxygentypedef:: BoltMode

.. doxygendefine:: BOLT_MODE_DIRECT

.. doxygendefine:: BOLT_MODE_ROUTING


BoltSocketOptions
-----------------

.. doxygentypedef:: BoltSocketOptions

.. doxygenfunction:: BoltSocketOptions_create

.. doxygenfunction:: BoltSocketOptions_destroy

.. doxygenfunction:: BoltSocketOptions_get_connect_timeout

.. doxygenfunction:: BoltSocketOptions_set_connect_timeout

.. doxygenfunction:: BoltSocketOptions_get_keep_alive

.. doxygenfunction:: BoltSocketOptions_set_keep_alive


BoltStat
--------

.. doxygenfunction:: BoltStat_memory_allocation_current

.. doxygenfunction:: BoltStat_memory_allocation_peak

.. doxygenfunction:: BoltStat_memory_allocation_events


BoltStatus
----------

.. doxygentypedef:: BoltStatus

.. doxygenfunction:: BoltStatus_create

.. doxygenfunction:: BoltStatus_destroy

.. doxygenfunction:: BoltStatus_get_state

.. doxygenfunction:: BoltStatus_get_error

.. doxygenfunction:: BoltStatus_get_error_context


BoltTransport
-------------

.. doxygentypedef:: BoltTransport

.. doxygendefine:: BOLT_TRANSPORT_PLAINTEXT

.. doxygendefine:: BOLT_TRANSPORT_ENCRYPTED


BoltTrust
---------

.. doxygentypedef:: BoltTrust

.. doxygenfunction:: BoltTrust_create

.. doxygenfunction:: BoltTrust_destroy

.. doxygenfunction:: BoltTrust_get_certs

.. doxygenfunction:: BoltTrust_set_certs

.. doxygenfunction:: BoltTrust_get_skip_verify

.. doxygenfunction:: BoltTrust_set_skip_verify

.. doxygenfunction:: BoltTrust_get_skip_verify_hostname

.. doxygenfunction:: BoltTrust_set_skip_verify_hostname


BoltValue
---------

.. doxygenenum:: BoltType

.. doxygentypedef:: BoltValue

.. doxygenfunction:: BoltValue_create

.. doxygenfunction:: BoltValue_destroy

.. doxygenfunction:: BoltValue_duplicate

.. doxygenfunction:: BoltValue_copy

.. doxygenfunction:: BoltValue_size

.. doxygenfunction:: BoltValue_type

.. doxygenfunction:: BoltValue_to_string

Null
^^^^

.. doxygenfunction:: BoltValue_format_as_Null

Boolean
^^^^^^^

.. doxygenfunction:: BoltValue_format_as_Boolean

.. doxygenfunction:: BoltBoolean_get

Integer
^^^^^^^

.. doxygenfunction:: BoltValue_format_as_Integer

.. doxygenfunction:: BoltInteger_get

Float
^^^^^

.. doxygenfunction:: BoltValue_format_as_Float

.. doxygenfunction:: BoltFloat_get

String
^^^^^^

.. doxygenfunction:: BoltValue_format_as_String

.. doxygenfunction:: BoltString_get

Dictionary
^^^^^^^^^^

.. doxygenfunction:: BoltValue_format_as_Dictionary

.. doxygenfunction:: BoltDictionary_key

.. doxygenfunction:: BoltDictionary_get_key

.. doxygenfunction:: BoltDictionary_get_key_size

.. doxygenfunction:: BoltDictionary_get_key_index

.. doxygenfunction:: BoltDictionary_set_key

.. doxygenfunction:: BoltDictionary_value

.. doxygenfunction:: BoltDictionary_value_by_key

List
^^^^

.. doxygenfunction:: BoltValue_format_as_List

.. doxygenfunction:: BoltList_resize

.. doxygenfunction:: BoltList_value

Bytes
^^^^^

.. doxygenfunction:: BoltValue_format_as_Bytes

.. doxygenfunction:: BoltBytes_get

.. doxygenfunction:: BoltBytes_get_all

Structure
^^^^^^^^^

.. doxygenfunction:: BoltValue_format_as_Structure

.. doxygenfunction:: BoltStructure_code

.. doxygenfunction:: BoltStructure_value
