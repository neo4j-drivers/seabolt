Building from Source
********************

Requirements
============

This project requires the following tools/libraries to be installed in order to be built.
General installation instructions can be found in the following sections.

1. A C Compiler Toolchain (GCC, CLang, MSVC)
2. CMake >= 3.12
3. OpenSSL Development Libraries (must include static libraries)

Linux
^^^^^

We have started using docker files for building binary packages that we release, which you can have a look at
https://github.com/neo4j-drivers/seabolt/ci/linux. You can have a look at those files to learn the exact steps
for a successful build process.

Ubuntu
++++++

1. Install all required components via `apt install git build-essential libssl-dev`
2. Default cmake package on Ubuntu is a bit out dated, so you need to have an up to date version. See
    https://cmake.org/download/ for downloads and installation instructions for your platform.

MacOS X
^^^^^^^

1. Install XCode
2. Install XCode Command Line Tools via `xcode-select --install`.
3. Install CMake `brew install cmake`.
4. Install OpenSSL `brew install openssl`.
5. Create an environment variable named `OPENSSL_ROOT_DIR` that points to the openssl library installation path
    (default is `/usr/local/opt/openssl`)

Windows
^^^^^^^

Currently windows builds also depend on OpenSSL, however support for windows secure sockets is in our roadmap.

MSVC
++++

1. Install Visual Studio 2017 with VC++ support
2. Install CMake via `choco install cmake --installargs 'ADD_CMAKE_TO_PATH=System'`
3. Install OpenSSL Win64 using one of the recent and stable 64-Bit binary builds (full version instead of light)
    listed at https://slproweb.com/products/Win32OpenSSL.html on a path with no spaces on it.

MINGW
+++++

1. Install MSYS2 from https://www.msys2.org/
2. Be sure to install a mingw toolchain into MSYS2 (you can use the following command
    `pacman -S --needed base-devel mingw-w64-i686-toolchain mingw-w64-x86_64-toolchain git subversion mercurial mingw-w64-i686-cmake mingw-w64-x86_64-cmake`.
3. Install OpenSSL Win64 using one of the recent and stable 64-Bit binary builds (full version instead of light)
    listed at https://slproweb.com/products/Win32OpenSSL.html on a path with no spaces on it.
