#!/usr/bin/env bash

BASE=$(dirname $0)

pushd ${BASE} 1> /dev/null
cmake -DCMAKE_BUILD_TYPE=Release .
EXIT_STATUS=$?
if [ "${EXIT_STATUS}" -eq "0" ]
then
    make
    EXIT_STATUS=$?
fi
popd 1> /dev/null
exit ${EXIT_STATUS}
