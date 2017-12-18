#!/usr/bin/env bash

BASE=$(dirname $0)

pushd ${BASE} 1> /dev/null
cmake -DCMAKE_BUILD_TYPE=Release .
make
popd 1> /dev/null
