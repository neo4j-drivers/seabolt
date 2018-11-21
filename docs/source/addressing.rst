==========
Addressing
==========

A :class:`BoltAddress` structure holds information about connection endpoints.
It will typically be initialised with a host name and and a port, which can later be resolved into one or more physical addresses.
The :func:`BoltAddress_create` function is used to create and initialise a :class: BoltAddress, the :func:`BoltAddress_resolve` function performs a blocking lookup on the host and port and may be repeated multiple times.
All resolved IP addresses are stored as IPv6 addresses, using the `IPv4 mapped address scheme <https://tools.ietf.org/html/rfc5156#section-2.2>`_ for IPv4 addresses.

Example:

.. code-block:: c

    struct BoltAddress* address = BoltAddress_create("graph.acme.com", "7687");
    BoltAddress_resolve(address);
    struct BoltConnection* connection = BoltConnection_open(BOLT_SECURE_SOCKET, address);
    // TODO
    BoltConnection_close(connection);
    BoltAddress_destroy(address);


-----------
BoltAddress
-----------

.. doxygentypedef:: BoltAddress

.. doxygenfunction:: BoltAddress_create

.. doxygenfunction:: BoltAddress_destroy

.. doxygenfunction:: BoltAddress_host

.. doxygenfunction:: BoltAddress_port

.. doxygenfunction:: BoltAddress_resolve

.. doxygenfunction:: BoltAddress_copy_resolved_host

--------------
BoltAddressSet
--------------

.. doxygentypedef:: BoltAddressSet

.. doxygenfunction:: BoltAddressSet_add

-------------------
BoltAddressResolver
-------------------

.. doxygentypedef:: address_resolver_func

.. doxygentypedef:: BoltAddressResolver

.. doxygenfunction:: BoltAddressResolver_create

.. doxygenfunction:: BoltAddressResolver_destroy
