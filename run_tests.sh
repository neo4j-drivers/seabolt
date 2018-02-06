#!/usr/bin/env bash

BASE=$(dirname $0)
PYTHON="python"
TEST_ARGS=$@

BOLTKIT_NOT_AVAILABLE=11
COMPILATION_FAILED=12
SERVER_INSTALL_FAILED=13
SERVER_CONFIG_FAILED=14
SERVER_START_FAILED=15
SERVER_STOP_FAILED=16
TESTS_FAILED=199

function check_boltkit
{
    echo "Checking boltkit"
    ${PYTHON} -c "import boltkit" 2> /dev/null
    if [ "$?" -ne "0" ]
    then
        echo "FATAL: The boltkit library is not available. Use \`pip install boltkit\` to install."
        exit ${BOLTKIT_NOT_AVAILABLE}
    fi
}

function compile
{
    echo "Compiling"
    ${BASE}/make_debug.sh
    if [ "$?" -ne "0" ]
    then
        echo "FATAL: Compilation failed."
        exit ${COMPILATION_FAILED}
    fi
}

function stop_server
{
    NEO4J_DIR=$1
    echo "-- Stopping server"
    neoctrl-stop "${NEO4J_DIR}"
    if [ "$?" -ne "0" ]
    then
        echo "FATAL: Failed to stop server."
        exit ${SERVER_STOP_FAILED}
    fi
    trap - EXIT
}

function run_tests
{
    NEO4J_VERSION=$1

    SERVER=${BASE}/build/server
    rm -r ${SERVER}

    echo "Testing against Neo4j ${NEO4J_VERSION}"

    echo "-- Installing server"
    NEO4J_DIR=$(neoctrl-install ${NEO4J_VERSION} ${SERVER})
    if [ "$?" -ne "0" ]
    then
        echo "FATAL: Server installation failed."
        exit ${SERVER_INSTALL_FAILED}
    fi
    echo "-- Server installed at ${NEO4J_DIR}"

    echo "-- Configuring server to accept IPv6 connections"
    neoctrl-configure "${NEO4J_DIR}" dbms.connectors.default_listen_address=::
    if [ "$?" -ne "0" ]
    then
        echo "FATAL: Unable to configure server for IPv6."
        exit ${SERVER_CONFIG_FAILED}
    fi

    echo "-- Setting initial password"
    neoctrl-set-initial-password "password" "${NEO4J_DIR}"
    if [ "$?" -ne "0" ]
    then
        echo "FATAL: Unable to set initial password."
        exit ${SERVER_CONFIG_FAILED}
    fi

    echo "-- Starting server"
    NEO4J_BOLT_URI=$(neoctrl-start ${NEO4J_DIR} | grep "^bolt:")
    if [ "$?" -ne "0" ]
    then
        echo "FATAL: Failed to start server."
        exit ${SERVER_START_FAILED}
    fi
    trap "stop_server ${NEO4J_DIR}" EXIT

    echo "-- Running tests"
    ${BASE}/build/bin/seabolt-test ${TEST_ARGS}
    if [ "$?" -ne "0" ]
    then
        echo "FATAL: Test execution failed."
        exit ${TESTS_FAILED}
    fi

    stop_server "${NEO4J_DIR}"

}

echo "Seabolt test run started at $(date -Ins)"
check_boltkit
compile
for NEO4J_VERSION in $(grep -E "^[0-9]+\.[0-9]+\.[0-9]+$" COMPATIBILITY)
do
    run_tests "${NEO4J_VERSION}"
done
echo "Seabolt test run completed at $(date -Ins)"
