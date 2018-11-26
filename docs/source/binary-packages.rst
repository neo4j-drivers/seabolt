Installing from binary packages
*******************************

Requirements
============

Currently, seabolt depends on **OpenSSL** on all platforms, without any exceptions.
For binary packages to function propertly, you need to have OpenSSL binaries installed for runtime support and
also install OpenSSL static libraries for building against static targets.


Where to get binary packages
============================

You can access latest released binary packages at `releases <https://github.com/neo4j-drivers/seabolt/releases>`_ page.
Download the package built for the platform you're using and make sure the sha256 checksum of the downloaded
file matches the published hash that is as well published with the package itself.

Installing
==========

Linux (Ubuntu)
^^^^^^^^^^^^^^

1. Install runtime dependencies by `sudo apt install -y libssl`.
2. Install development dependencies (if you're building against static libraries) `sudo apt install -y libssl-dev`.
3. Install seabolt by `sudo dpkg -i <Downloaded File.deb>`.

Linux (CentOS)
^^^^^^^^^^^^^^

1. Install runtime dependencies by `sudo yum -y install openssl`.
2. Install development dependencies (if you're building against static libraries) `sudo yum -y install openssl-static`.
3. Install seabolt by `sudo rpm -iVh <Downloaded File.rpm>`.

Linux (Alpine)
^^^^^^^^^^^^^^

1. Install runtime dependencies by `apk add openssl`.
2. Install development dependencies (if you're building against static libraries) `apk add openssl-dev`.
3. Install seabolt by extracting the archive into `/` by `tar zxv -C / <Downloaded File.tar.gz>`.

MacOS X
^^^^^^^

1. Install dependencies by `brew install openssl`.
2. Install seabolt by extracting the archive into `/` by `tar zxv -C / <Downloaded File.tar.gz>`

Windows x64
^^^^^^^^^^^

1. Install OpenSSL by following instructions https://slproweb.com/products/Win32OpenSSL.html.
2. Install seabolt by extracting the archive into a folder of your choice, i.e. `C:\Seabolt17` and
    add `C:\Seabolt17\bin` into your PATH environment variable.