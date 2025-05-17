#
# Stage 1 - Frontend Build
#
FROM node:24 AS frontend-builder
WORKDIR /app/client

COPY ./client/ .

RUN npm install && npm run build 

#
# Stage 2 - cc_server Build
#
FROM alpine:latest AS cc_server-builder
WORKDIR /app

COPY . /app

RUN apk update &&\
    apk add \
    musl-dev \
    pkgconfig \
    gcc \
    g++ \
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
    cmake \
    hiredis-dev

RUN meson setup build --buildtype debug
RUN ninja -C build
RUN mkdir /deps && ldd /app/build/backend/server/cc_server | awk '{print $3}' | xargs -I '{}' cp -v '{}' /deps/

#
# Stage 3 - Final Stage
#
FROM alpine:latest 
WORKDIR /app 

COPY --from=cc_server-builder /app/build/backend/server/cc_server /app/backend/cc_server
COPY --from=cc_server-builder /app/backend/sql /app/backend/sql
COPY --from=cc_server-builder /app/backend/server/src /app/backend/server/src
COPY --from=cc_server-builder /app/backend/server/include /app/backend/server/include
COPY --from=cc_server-builder /deps /lib/
COPY --from=frontend-builder /app/client/public /app/client/public

RUN apk --no-cache add file

EXPOSE 443

CMD ["backend/cc_server"]
