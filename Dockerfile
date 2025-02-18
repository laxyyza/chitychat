FROM ubuntu:latest

RUN apt-get update && apt-get install -y\
    build-essential \
    libpq-dev \
    postgresql \
    postgresql-contrib \
    git \
    curl \
    wget \
    meson \
    sudo \
    pkg-config \
    libjson-c-dev \
    libmagic-dev \
    && rm -rf /var/lib/apt/lists/*

RUN service postgresql start && \
    sudo -u postgres psql -c "CREATE USER root WITH PASSWORD 'secret';" && \
    sudo -u postgres psql -c "CREATE DATABASE chitychat;" && \
    sudo -u postgres psql -c "GRANT ALL PRIVILEGES ON DATABASE chitychat TO root;" && \
    sudo -u postgres psql -c "ALTER USER root CREATEDB;" && \
    sudo -u postgres psql -d chitychat -c "ALTER SCHEMA public OWNER TO root;" && \
    sudo -u postgres psql -d chitychat -c "GRANT ALL ON SCHEMA public TO root;"

WORKDIR /app

COPY . /app

RUN meson setup build
RUN ninja -C build

EXPOSE 443

CMD service postgresql start && ./build/chitychat
