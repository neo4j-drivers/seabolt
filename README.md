# Seabolt

Seabolt is the C Connector Library for Neo4j.
The library supports multiple versions of the Bolt protocol through the new _Connector API_ and will provide a base layer for a number of language drivers.

## Installing from packages

### Linux (Ubuntu)

1. Install runtime dependencies
```
# you can also choose to install libssl1.1
sudo apt-get install -y libssl1.0.0 
```

2. Fetch the latest package (check [here](https://github.com/neo4j-drivers/seabolt/releases) for latest releases)
```
wget https://github.com/neo4j-drivers/seabolt/releases/download/v1.7.4/seabolt-1.7.4-Linux-ubuntu-$(lsb_release -rs).deb
```

3. (optional) Check the sha256 hash of the download matches the published hash

4. Install the package and clean up
```
dpkg -i seabolt-1.7.4-Linux-ubuntu-$(lsb_release -rs).deb
rm seabolt-1.7.4-Linux-ubuntu-$(lsb_release -rs).deb
```

### Linux and MacOS (tar.gz)

1. Seabolt requires openssl libraries to be present on the system. Most linux distributions have them installed by default and you can use `brew install openssl` to install it on MacOS. If you 

2. Download the package you want to install (check [here](https://github.com/neo4j-drivers/seabolt/releases) for latest releases).

3. Extract the package using the following command (assuming that you saved the package as `~/Downloads/seabolt.tar.gz`). This command actually skips the top-level folder inside the tar.gz package and extracts its contents into the root folder `/`.
```
tar zxvf ~/Downloads/seabolt.tar.gz --strip-components=1 -C /
```

### Windows (zip)

1. Download the package you want to install (check [here](https://github.com/neo4j-drivers/seabolt/releases) for latest releases).

2. Extract the package using the following powershell script (assuming that you saved the package as `C:\Temp\seabolt.zip`).
```
Expand-Archive -Path C:\Temp\seabolt.zip -DestinationPath C:\
Get-ChildItem -Path C:\seabolt-* -Directory | Rename-Item -NewName seabolt
```

## Building from source

This project requires the following tools/libraries to be installed in order to be built. General installation instructions can be found in the following sections.

1. CMake >= 3.12
2. For **_non-Windows_** systems OpenSSL Development Libraries (must include static libraries)

### Linux (Ubuntu)

1. Install all required components via ```apt install git build-essential cmake libssl-dev```

### MacOS X

1. Install XCode,
2. Install XCode Command Line Tools via `xcode-select --install`
3. Install CMake `brew install cmake`
4. Install OpenSSL `brew install openssl`
5. Create an environment variable named `OPENSSL_ROOT_DIR` that points to the openssl library installation path (default is `/usr/local/opt/openssl`)

### Windows

#### MSVC

1. Install Visual Studio 2017 with VC++ support
2. Install CMake via `choco install cmake --installargs 'ADD_CMAKE_TO_PATH=System'`

#### MINGW

1. Install MSYS2 from `https://www.msys2.org/`
    * Be sure to install a mingw toolchain into MSYS2 (you can use the following command `pacman -S --needed base-devel mingw-w64-i686-toolchain mingw-w64-x86_64-toolchain git subversion mercurial mingw-w64-i686-cmake mingw-w64-x86_64-cmake`)

## Building

### Linux / MacOS X

To build the project, run either the `make_debug.sh` or the `make_release.sh` script from the project root directory.
This will compile and deposit project artifacts in the `build/dist` directory.

To create distributable packages, invoke `cpack` in `build` directory (after `make_debug.sh` or `make_release.sh` is completed) and all binary package artifacts will be placed in `build/dist-package` directory.

### Windows

#### MINGW

To build the project, run either the `make_debug.cmd` or the `make_release.cmd` script from the project root directory with `MINGW` as its first argument.
This will compile and deposit project artifacts in the `build/dist` directories.

```
make_[debug|release].cmd MINGW
```

To create distributable packages, invoke `make.cmd` with your preferred arguments and all binary package artifacts will be placed in `build/dist-package` directory.

```
make.cmd MINGW [Debug|Release] x64 package
```

#### MSVC

To build the project, run either the `make_debug.cmd` or the `make_release.cmd` script from the project root directory with `MSVC` as its first argument.
This will compile and deposit project artifacts in the `build/dist` directories.

```
make_[debug|release].cmd MSVC
```

To create distributable packages, invoke `make.cmd` with your preferred arguments and all binary package artifacts will be placed in `build/dist-package` directory.

```
make.cmd MSVC [Debug|Release] x64 package
```

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
BOLT_PASSWORD=password build/bin/seabolt-cli run "UNWIND range(1, 1000000) AS n RETURN n"
```

By default, this will simply display stats for the query execution.
The following environment variables can be used:
```
BOLT_ROUTING=0|1
BOLT_ACCESS_MODE=WRITE|READ
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
build/bin/(Debug|Release)\seabolt-cli.exe run "UNWIND range(1, 1000000) AS n RETURN n"
```

By default, this will simply display stats for the query execution.
The following environment variables can be used (with Powershell syntax):
```
$env:BOLT_ROUTING=0|1
$env:BOLT_ACCESS_MODE=WRITE|READ
$env:BOLT_SECURE=0|1
$env:BOLT_HOST=<host name, IPv4 or IPv6 address>
$env:BOLT_PORT=7687
$env:BOLT_USER=neo4j
$env:BOLT_PASSWORD=password
$env:BOLT_LOG=0|1|2
```
