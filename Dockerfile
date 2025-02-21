FROM alpine:latest

RUN echo "nameserver 8.8.8.8" > /etc/resolv.conf &&\
    apk update &&\
    apk add musl-dev pkgconfig gcc meson openssl openssl-dev json-c json-c-dev libpq libpq-dev file file-dev linux-headers cmake

WORKDIR /app

COPY . /app

RUN meson setup build
RUN ninja -C build
RUN apk del musl-dev pkgconfig gcc meson openssl-dev json-c-dev libpq-dev file-dev linux-headers cmake
RUN mkdir bin &&\
    cp -v build/chitychat bin/ &&\
    rm -rvf server/src server/include build

EXPOSE 8080

CMD ./bin/chitychat -R