# Seabolt

Seabolt is a Neo4j connector library for C.
The library will support multiple versions of the Bolt protocol through the new _Connector API_ and will provide a base layer for a number of language drivers.

This project is a work in progress, so we're not yet able to accept issues or contributions. Sorry!

## Requirements

This project requires the following tools/libraries to be installed in order to be built. General installation instructions can be found in the following sections.

1. CMake 3.9.1
2. OpenSSL Development Libraries

### Linux (Ubuntu)

1. Install all required components via ```apt install git build-essential cmake libssl-dev```

### MacOS X

1. Install XCode,
2. Install XCode Command Line Tools via `xcode-select --install`
3. Install CMake `brew install cmake`
4. Install OpenSSL `brew install openssl`
5. Create an environment variable named `OPENSSL_LIB_ROOT` that points to the openssl library installation path (default is `/usr/local/opt/openssl`)

### Windows

Currently windows builds also depend on OpenSSL, however support for windows secure sockets is in our roadmap.

1. Install Visual Studio 2017 with VC++ support,
2. Install CMake via `choco install cmake --installargs 'ADD_CMAKE_TO_PATH=System'`
3. Install OpenSSL Win32 using `https://slproweb.com/download/Win32OpenSSL-1_1_0g.exe` 
4. Install OpenSSL Win64 using `https://slproweb.com/download/Win64OpenSSL-1_1_0g.exe`
5. Create an environment variable named `OPENSSL_LIB_ROOT` that points to the openssl library installation path (default is `C:/OpenSSL-Win64`).

## Building

### Linux / MacOS X

To build the project, run either the `make_debug.sh` or the `make_release.sh` script from the project root directory.
This will compile and deposit project artifacts in the `build/bin` and `build/lib` directories.
To create distributable packages, use `make_packages.sh` instead.

### Windows

To build the project, run either the `make_debug.cmd` or the `make_release.cmd` script from the project root directory.
This will compile and deposit project artifacts in the `build/bin` and `build/lib` directories.

## Docs 

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

### Linux / MacOS X

To run a query, use the following...
```
BOLT_PASSWORD=password build/bin/bolt "UNWIND range(1, 1000000) AS n RETURN n"
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

### Windows (Powershell)

To run a query, use the following...
```
$env:BOLT_PASSWORD=password
build/bin/bolt.exe "UNWIND range(1, 1000000) AS n RETURN n"
```

By default, this will simply display stats for the query execution.
The following environment variables can be used:
```
$env:BOLT_SECURE=0|1
$env:BOLT_HOST=<host name, IPv4 or IPv6 address>
$env:BOLT_PORT=7687
$env:BOLT_USER=neo4j
$env:BOLT_PASSWORD=password
$env:BOLT_LOG=0|1|2
```
