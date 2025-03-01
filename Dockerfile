FROM alpine:latest

RUN apk update &&\
    apk add \
    musl-dev \
    pkgconfig \
    gcc \
    meson \
    openssl \
    openssl-dev \
    json-c \
    json-c-dev \
    libpq \
    libpq-dev \
    file \
    file-dev \
    linux-headers \
    cmake

WORKDIR /app

COPY . /app

RUN meson setup build --debug --buildtype plain
RUN ninja -C build
RUN apk del musl-dev pkgconfig gcc meson openssl-dev json-c-dev libpq-dev file-dev linux-headers cmake
RUN mkdir bin coredumps &&\
    cp -v build/chitychat bin/ &&\
    rm -rvf build

EXPOSE 8080

CMD ./bin/chitychat -R
