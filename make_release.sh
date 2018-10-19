#!/usr/bin/env bash

BASE=$(dirname $0)
BUILD=${BASE}/build

mkdir -p ${BUILD} 1>/dev/null

pushd ${BUILD} 1> /dev/null
cmake -D CMAKE_BUILD_TYPE=Release -D CMAKE_INSTALL_PREFIX=dist  ..
EXIT_STATUS=$?
if [ "${EXIT_STATUS}" -eq "0" ]
then
    cmake --build . --target install
    EXIT_STATUS=$?
fi
popd 1> /dev/null
exit ${EXIT_STATUS}
