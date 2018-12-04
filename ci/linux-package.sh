#!/usr/bin/env bash

BASE=$(dirname $0)

if [[ -z "$1" ]]; then
    echo "Please provide the name of distro that you want the packages to be generated for"
    exit 1
fi

if [[ ! -f "$BASE/linux/Dockerfile.package-$1" ]]; then
    echo "Unable to find docker file for $1 at $BASE/linux/Dockerfile.package-$1"
    exit 1
fi

docker image build --build-arg SEABOLT_VERSION=$SEABOLT_VERSION -t seabolt-package -f $BASE/linux/Dockerfile.package-$1 $BASE/..
if [[ "$?" -ne "0" ]]; then
    echo "FATAL: docker image build failed, possible compilation failure."
    exit 1
fi

docker container run --rm seabolt-package | tar xvz --strip 1
if [[ "$?" -ne "0" ]]; then
    echo "FATAL: docker container run failed, possible packaging failure."
    exit 1
fi
