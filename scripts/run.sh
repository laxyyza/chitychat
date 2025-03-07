#!/usr/bin/env sh

# This script runs ChityChat as a child process instead of PID 1.
# If a process runs as PID 1 inside a Docker container, it ignores
# default signal handling behavior, including SIGSEGV.
#
# By running it as a child process, I can send SIGSEGV if the server stalls,
# ensuring a proper core dump for debugging.

bin/chitychat -R $@ &
pid=$!

forward_signal() {
    kill -$1 $pid
}

trap 'forward_signal SIGTERM' SIGTERM
trap 'forward_signal SIGINT' SIGINT

wait $pid

exit $?
