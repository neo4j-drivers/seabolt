=====================
Connection Management
=====================

::

    struct BoltAddress* address = BoltAddress_create("graph.acme.com", "7687");
    BoltAddress_resolve_b(address);
    struct BoltConnection* connection = BoltConnection_open_b(BOLT_SECURE_SOCKET, address);
    // TODO
    BoltConnection_close_b(connection);
    BoltAddress_destroy(address);


Addressing
==========

A :class:`BoltAddress` structure holds details of a Bolt server address.
This structure will typically be initialised with a logical address (i.e. a domain name) that can later be resolved into one or more physical IP addresses.
The :func:`BoltAddress_create` function is used to create and initialise a structure, the :func:`BoltAddress_resolve_b` function performs a blocking lookup on the host and port and may be repeated multiple times.
All resolved IP addresses are stored as IPv6 addresses, using the `IPv4 mapped address scheme <https://tools.ietf.org/html/rfc5156#section-2.2>`_ for IPv4 addresses.

.. doxygenstruct:: BoltAddress
   :members:

.. doxygenfunction:: BoltAddress_create

.. doxygenfunction:: BoltAddress_resolve_b

.. doxygenfunction:: BoltAddress_resolved_host

.. doxygenfunction::  BoltAddress_resolved_host_is_ipv4

.. doxygenfunction:: BoltAddress_destroy


Connections
===========

Connections provide the core means of communication to a Bolt server.

.. doxygenstruct:: BoltConnection
   :members:

.. doxygenenum:: BoltTransport

.. doxygenfunction:: BoltConnection_open_b

.. doxygenfunction:: BoltConnection_close_b

.. doxygenfunction:: BoltConnection_init_b

.. doxygenfunction:: BoltConnection_send_b

.. doxygenfunction:: BoltConnection_receive_b

.. doxygenfunction:: BoltConnection_fetch_summary_b

.. doxygenfunction:: BoltConnection_fetch_b
