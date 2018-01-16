#!/usr/bin/env bash

BASE=$(dirname $0)

pushd ${BASE} 1> /dev/null
${BASE}/make_debug.sh
EXIT_STATUS=$?
if [ "${EXIT_STATUS}" -eq "0" ]
then
    build/bin/seabolt-test $@
    EXIT_STATUS=$?
fi
popd 1> /dev/null
exit ${EXIT_STATUS}
