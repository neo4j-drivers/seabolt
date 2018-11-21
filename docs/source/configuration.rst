=============
Configuration
=============

There are several configurable behaviours that you can tweak both at the connector and connection level.

-----------------
BoltConfig
-----------------

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

---------
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

-----------------
BoltSocketOptions
-----------------

.. doxygentypedef:: BoltSocketOptions

.. doxygenfunction:: BoltSocketOptions_create

.. doxygenfunction:: BoltSocketOptions_destroy

.. doxygenfunction:: BoltSocketOptions_get_connect_timeout

.. doxygenfunction:: BoltSocketOptions_set_connect_timeout

.. doxygenfunction:: BoltSocketOptions_get_keep_alive

.. doxygenfunction:: BoltSocketOptions_set_keep_alive
