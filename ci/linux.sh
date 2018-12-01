#!/usr/bin/env bash

set -e

BASE=$(dirname $0)

if [ "x${NEOCTRL_LOCAL_PACKAGE}" != "x" ]; then
    NLPPATH=/tmp/neo4j/${NEOCTRL_LOCAL_PACKAGE}
    mkdir -p $(dirname $NLPPATH) && cp ${NEOCTRL_LOCAL_PACKAGE} ${NLPPATH}
    NLPVOLUME="$(dirname $NLPPATH):/opt/neo4j"
    NEOCTRL_LOCAL_PACKAGE="/opt/neo4j/$(basename $NLPPATH)"
    DOCKER_RUN_ARGS="${DOCKER_RUN_ARGS} -v ${NLPVOLUME} --env NEOCTRL_LOCAL_PACKAGE=${NEOCTRL_LOCAL_PACKAGE}"
fi

docker image build -t seabolt-ci -f $BASE/linux/Dockerfile.build $BASE/..
if [[ "$?" -ne "0" ]]; then
    echo "FATAL: docker image build failed, possible compilation failure."
    exit 1
fi

docker container run --rm --env TEAMCITY_HOST="$TEAMCITY_HOST" --env TEAMCITY_USER="$TEAMCITY_USER" \
    --env TEAMCITY_PASSWORD="$TEAMCITY_PASSWORD" --env NEOCTRLARGS="$NEOCTRLARGS" \
    --env BOLT_PORT="7687" --env HTTP_PORT="7474" --env HTTPS_PORT="7473" \
    --env BOLT_PASSWORD="password" $DOCKER_RUN_ARGS seabolt-ci
if [[ "$?" -ne "0" ]]; then
    echo "FATAL: docker container run failed, possible test failure."
    exit 1
fi
