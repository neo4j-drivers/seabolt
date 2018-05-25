==========
Addressing
==========

A :class:`BoltAddress` structure holds details of a Application server address.
This structure will typically be initialised with a logical address (i.e. a domain name) that can later be resolved into one or more physical IP addresses.
The :func:`BoltAddress_create` function is used to create and initialise a structure, the :func:`BoltAddress_resolve` function performs a blocking lookup on the host and port and may be repeated multiple times.
All resolved IP addresses are stored as IPv6 addresses, using the `IPv4 mapped address scheme <https://tools.ietf.org/html/rfc5156#section-2.2>`_ for IPv4 addresses.

Example:

.. code-block:: c

    struct BoltAddress* address = BoltAddress_create("graph.acme.com", "7687");
    BoltAddress_resolve(address);
    struct BoltConnection* connection = BoltConnection_open(BOLT_SECURE_SOCKET, address);
    // TODO
    BoltConnection_close(connection);
    BoltAddress_destroy(address);


.. doxygenstruct:: BoltAddress
    :members:

.. doxygenfunction:: BoltAddress_create

.. doxygenfunction:: BoltAddress_resolve

.. doxygenfunction:: BoltAddress_copy_resolved_host

.. doxygenfunction:: BoltAddress_destroy
