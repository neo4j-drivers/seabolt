Test PR build. Delete me once test passed.

# Seabolt

Seabolt is a Neo4j connector library for C.
The library will support multiple versions of the Bolt protocol through the new _Connector API_ and will provide a base layer for a number of language drivers.

This project is a work in progress, so we're not yet able to accept issues or contributions. Sorry!


## Building

To build the project, run either the `make_debug.sh` or the `make_release.sh` script from the project root directory.
This will compile and deposit project artifacts in the `bin` and `lib` directories.

To build the docs, the `make_docs.sh` script is available.
The docs build process uses [Sphinx](http://www.sphinx-doc.org/), [Breathe](https://breathe.readthedocs.io/) and [Doxygen](http://www.doxygen.org/).
These can be installed as follows:

```
$ sudo apt install doxygen
```

```
$ pip install --user sphinx breathe
```


## Running

To run a query, use the following...
```
BOLT_PASSWORD=password bin/seabolt "UNWIND range(1, 1000000) AS n RETURN n"
```

By default, this will simply display stats for the query execution.
The following environment variables can be used:
```
BOLT_SECURE=0|1
BOLT_HOST=<host name, IPv4 or IPv6 address>
BOLT_PORT=7687
BOLT_USER=neo4j
BOLT_PASSWORD=password
BOLT_LOG=0|1|2
```
