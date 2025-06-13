#!/usr/bin/env bash

export VERSION=$(cat VERSION)
export TAG=:$VERSION
export OWNER_NAME=chitychat
GO_CMD_DIR=backend/services/go/cmd

function build_cc_server() 
{
    echo -e "\nBuilding cc_server${TAG} docker image...\n"
    docker build -t ${OWNER_NAME}/cc_server${TAG} -t ${OWNER_NAME}/cc_server:latest . || exit $?
}

function build_go_image()
{
    SERVICE=$1
    IMAGE_NAME=${OWNER_NAME}/${SERVICE}

    echo -e "\nBuilding ${IMAGE_NAME}${TAG} docker image...\n"
    docker build --build-arg SERVICE=${SERVICE} -t ${IMAGE_NAME}${TAG} -t ${IMAGE_NAME}:latest -f backend/services/go/Dockerfile . || exit $?
}

function build_all_go_images()
{
    echo "Building all GO backend service images..."

    for f in backend/services/go/cmd/*; do
        SERVICE=$(basename $f)

        build_go_image $SERVICE
    done
}

function validate_args()
{
    for ARG in $@; do
        [[ -d "${GO_CMD_DIR}/${ARG}" ]] || { echo "No such service: '$ARG'"; exit -1; }
    done
}

function build_specific_services()
{
    validate_args $@

    for ARG in $@; do 
        build_go_image $ARG
    done
}

if [[ $1 == 'cc_server' ]]; then
    build_cc_server
elif [[ $1 == 'go' ]]; then
    build_all_go_images
elif [[ $# -eq 0 ]]; then
    build_cc_server 
    build_all_go_images
else
    build_specific_services $@
fi
