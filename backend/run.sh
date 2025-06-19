#!/usr/bin/env bash

START_SERVER=${START_SERVER:=1}
BUILD_DIR=${BUILD_DIR:=build}
SERVER_EXE=$BUILD_DIR/backend/server/cc_server
SERVICES_DIR=$BUILD_DIR/backend/services


SERVICE_PIDS=()
SERVER_PID=
STOPPED=0
ARGV=$@

function exec_services()
{
    for service_exe in $SERVICES_DIR/*; do
        if [ -x $service_exe ]; then
            echo "Executing $(basename $service_exe)..."
            $service_exe &
            SERVICE_PIDS+=($!)
        fi
    done
}

function exec_server()
{
    echo "Executing $(basename $SERVER_EXE)"
    $SERVER_EXE $ARGV &
    SERVER_PID=$!
}

function cleanup()
{
    [[ $STOPPED -eq 1 ]] && return 0

    echo "Stopping all services..."
    kill ${SERVICE_PIDS[@]} $SERVER_PID
    wait
    echo "All services stopped"
    STOPPED=1
}

function main() 
{
    trap cleanup SIGINT SIGTERM

    export $(cat .env | xargs)

    if [[ $START_SERVER -eq 1 ]]; then
        exec_server
        sleep 0.1
    fi

    exec_services

    if [[ $START_SERVER -eq 1 ]]; then
        wait $SERVER_PID
        cleanup
    fi
}

main
