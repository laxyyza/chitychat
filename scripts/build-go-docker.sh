#!/usr/bin/env bash

OWNER_NAME=${OWNER_NAME:-chitychat}

function build_image()
{
    SERVICE=$1
    IMAGE_NAME=${OWNER_NAME}/${SERVICE}

    echo -e "\nBuilding ${IMAGE_NAME}${TAG} docker image...\n"
    docker build --build-arg SERVICE=${SERVICE} -t ${IMAGE_NAME}${TAG} -t ${IMAGE_NAME}:latest -f backend/services/go/Dockerfile . || exit $?
}

function build_all_images()
{
    for f in backend/services/go/cmd/*; do
        SERVICE=$(basename $f)

        build_image $SERVICE
    done
}

if [[ $1 ]]; then
    build_image $1
else 
    build_all_images
fi
