FROM golang:alpine AS builder

ARG SERVICE

WORKDIR /app
COPY backend/services/go .

RUN go build -o /${SERVICE} ./cmd/${SERVICE}

FROM alpine:latest
WORKDIR /app
ARG SERVICE
COPY --from=builder /${SERVICE} /usr/bin/${SERVICE}
COPY backend/sql /app/backend/sql
