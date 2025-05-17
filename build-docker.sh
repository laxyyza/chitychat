#!/usr/bin/env bash

GO_DIR=backend/services/go

function build_cc_server() 
{
    echo -e "\nBuilding cc_server docker image...\n"
    docker build -t chitychat/cc_server . || exit $?
}

function build_go_services()
{
    echo -e "\nBuilding go backend services docker images...\n"
    cd $GO_DIR
    ./build-docker.sh || exit $?
}

build_cc_server 
build_go_services
