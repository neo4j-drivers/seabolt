#!/usr/bin/env bash

BASE=$(dirname $0)
DOCS=${BASE}/docs

pushd ${DOCS} 1> /dev/null
doxygen Doxyfile
make html
popd 1> /dev/null
