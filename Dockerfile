FROM ubuntu:latest

RUN apt-get update && apt-get install -y\
    build-essential \
    libpq-dev \
    meson \
    sudo \
    pkg-config \
    libjson-c-dev \
    libmagic-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY . /app

RUN meson setup build
RUN ninja -C build

EXPOSE 8080

CMD ./build/chitychat -R
