#!/usr/bin/env bash


BASE=$(dirname $0)


COMPILATION_FAILED=12
PACKAGING_FAILED=19


function compile_release
{
    echo "Compiling for release"
    make clean
    cmake -DCMAKE_BUILD_TYPE=Release .
    if [ "$?" -ne "0" ]
    then
        echo "FATAL: Compilation failed."
        exit ${COMPILATION_FAILED}
    fi
    make
    if [ "$?" -ne "0" ]
    then
        echo "FATAL: Compilation failed."
        exit ${COMPILATION_FAILED}
    fi
}


function package
{
    echo "Packaging"
    cpack
    if [ "$?" -ne "0" ]
    then
        echo "FATAL: Packaging failed."
        exit ${PACKAGING_FAILED}
    fi
    lintian ${BASE}/seabolt/build/pkg/*.deb
    if [ "$?" -ne "0" ]
    then
        echo "FATAL: Packaging failed."
        exit ${PACKAGING_FAILED}
    fi
}


# TODO: fix lintian failures
compile_release
package
