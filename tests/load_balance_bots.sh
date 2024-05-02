#!/usr/bin/env bash

cleanup()
{
    echo "Ctrl+C pressed. Killing..."
    kill $(jobs -p)
    exit 0
}

trap cleanup SIGINT

START=0
END=0

[[ $1 ]] && START=$1
[[ $2 ]] && END=$2

[[ $END == 0 ]] && END=$START && START=0

BOT_CMD=./client/headless/bot_v1.py 

for ((i=$START; i < $END; i++))
do
    echo "Executing $BOT_CMD $i" 
    $BOT_CMD $i 1>/dev/null &
    sleep 0.1
done

wait
