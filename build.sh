#!/usr/bin/env sh

BUILD_DIR=${BUILD_DIR:=build}
GO_DIR=${GO_DIR:=backend/services/go}
SERVICES_OUT=${SERVICES_OUT:=$BUILD_DIR/backend/services}
FRONTEND_DIR=${FRONTEND_DIR:=client}

echo "BUILD_DIR: ${BUILD_DIR}"

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

build_server
build_go
build_frontend
