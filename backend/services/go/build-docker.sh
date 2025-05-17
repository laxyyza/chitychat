#!/usr/bin/env bash

function build_image()
{
    SERVICE=$1

    echo -e "\nBuilding $SERVICE docker image...\n"
    docker build --build-arg SERVICE=${SERVICE} -t chitychat/${SERVICE} -f Dockerfile . || exit $?
}

function build_all_images()
{
    for f in cmd/*; do
        SERVICE=$(basename $f)

        build_image $SERVICE
    done
}

if [[ $1 ]]; then
    build_image $1
else 
    build_all_images
fi
