#!/usr/bin/env bash

export VERSION=$(cat VERSION)
export TAG=:$VERSION
export OWNER_NAME=chitychat

function build_cc_server() 
{
    echo -e "\nBuilding cc_server${TAG} docker image...\n"
    docker build -t ${OWNER_NAME}/cc_server${TAG} -t ${OWNER_NAME}/cc_server:latest . || exit $?
}

function build_go_services()
{
    echo -e "\nBuilding go backend services docker images...\n"
    ./scripts/build-go-docker.sh || exit $?
}

if [[ $1 == 'cc_server' ]]; then
    build_cc_server
elif [[ $1 == 'go' ]]; then
    build_go_services
else
    build_cc_server 
    build_go_services
fi
