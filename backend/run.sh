#!/usr/bin/env bash

BUILD_DIR=${BUILD_DIR:=build}
SERVER_EXE=$BUILD_DIR/backend/server/cc_server
SERVICES_DIR=$BUILD_DIR/backend/services

SERVICE_PIDS=()
SERVER_PID=
STOPPED=0

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
    $SERVER_EXE &
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

trap cleanup SIGINT SIGTERM

exec_server
sleep 0.1
exec_services

wait $SERVER_PID

cleanup
