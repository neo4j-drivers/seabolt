#!/usr/bin/env bash

BASE=$(dirname $0)

pushd ${BASE} 1> /dev/null
source make_debug.sh
bin/seabolt-test $@
popd 1> /dev/null
