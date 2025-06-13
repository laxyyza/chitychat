#!/usr/bin/env sh

ARG=${1-all}
BUILD_DIR=${BUILD_DIR:=build}
GO_DIR=${GO_DIR:=backend/services/go}
SERVICES_OUT=${SERVICES_OUT:=$BUILD_DIR/backend/services}
FRONTEND_DIR=${FRONTEND_DIR:=client}
BUILD_DOCKER=${BUILD_DOCKER:=./scripts/build-docker.sh}

function build_go() 
{
    echo "Building go $GO_DIR"
    PREV_PWD=$(pwd)
    OUTPUT=$(realpath $SERVICES_OUT)
    cd $GO_DIR || exit -1
    echo "go build -o $OUTPUT ./cmd/*"
    go build -o $OUTPUT ./cmd/* || exit -1
    cd $PREV_PWD || exit -1
}

function build_frontend()
{
    echo "Building frontend react"
    PREV_PWD=$(pwd)
    cd $FRONTEND_DIR || exit -1
    npm run build || exit -1
    cd $PREV_PWD || exit -1
}

function build_server() 
{
    ninja -C $BUILD_DIR || exit -1
    mkdir -p $SERVICES_OUT
}

function print_help()
{
    echo "
Usage:
    $0 [COMMAND]

COMMANDS:
    cc_server   -   Build 'cc_server' (backend/server/)
    backend     -   Build backend 'cc_server' + Go services (backend/)
    frontend    -   Build frontend (client/)
    go          -   BUild Go backend services (backend/services/go/)
    image       -   Run '$BUILD_DOCKER' with \$2+ as arguments
    help        -   Print this message
    default     -   Build backend and frontend (default when no arguments)
    "
    exit 1
}

case $ARG in
    cc_server)
        build_server
        ;;
    backend)
        build_server
        build_go
        ;;
    frontend)
        build_frontend
        ;;
    go)
        build_go
        ;;
    image)
        shift 1
        exec ./scripts/build-docker.sh $@
        ;;
    default)
        build_server
        build_go
        build_frontend
        ;;
    help)
        print_help
        ;;
    *)
        echo "Unkown service: '$ARG'"
        exit -1
        ;;
esac
