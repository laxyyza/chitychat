#!/usr/bin/env bash

exec valgrind \
    --suppressions=backend/server/valgrind/glibc-ld.supp \
    --leak-check=full \
    --show-leak-kinds=all \
    --track-origins=yes \
    build/backend/server/cc_server $@
