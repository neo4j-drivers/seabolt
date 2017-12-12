# Seabolt

Seabolt is a Neo4j connector library for C.

This project is a work in progress and will support both Bolt v1 and v2 through the new _Connector API_.


## Building

To build the project, run the code below from the root project directory.
```bash
$ cmake . && make
```

This will compile and deposit the project artifacts in the `bin` and `lib` directories.


## Running

To run a query, use the following...
```
BOLT_PASSWORD=password bin/seabolt "UNWIND range(1, 1000000) AS n RETURN n"
```

By default, this will simply display stats for the query execution.
The following environment variables can be used:
```
BOLT_SECURE=1|0
BOLT_HOST=<host name, IPv4 or IPv6 address>
BOLT_PORT=7687
BOLT_USER=neo4j
BOLT_PASSWORD=password
```
