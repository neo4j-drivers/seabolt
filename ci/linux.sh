#!/usr/bin/env bash

BASE=$(dirname $0)

docker image build -t seabolt-ci -f $BASE/linux/Dockerfile.build $BASE/..
if [[ "$?" -ne "0" ]]; then
    echo "FATAL: docker image build failed, possible compilation failure."
    exit 1
fi

docker container run --rm --env TEAMCITY_HOST="$TEAMCITY_HOST" --env TEAMCITY_USER="$TEAMCITY_USER" \
    --env TEAMCITY_PASSWORD="$TEAMCITY_PASSWORD" --env NEOCTRLARGS="$NEOCTRLARGS" \
    --env BOLT_PORT="7687" --env HTTP_PORT="7474" --env HTTPS_PORT="7473" \
    --env BOLT_PASSWORD="password" seabolt-ci
if [[ "$?" -ne "0" ]]; then
    echo "FATAL: docker container run failed, possible test failure."
    exit 1
fi