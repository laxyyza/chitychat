#!/usr/bin/env bash

cleanup()
{
    echo "Ctrl+C pressed. Killing..."
    kill $(jobs -p)
    exit 0
}

trap cleanup SIGINT

N_BOTS=10

[[ $1 ]] && N_BOTS=$1

BOT_CMD=./client/headless/bot_v1.py 

for ((i=0; i < $N_BOTS; i++))
do
    echo "Executing $BOT_CMD $i" 
    $BOT_CMD $i 1>/dev/null &
    #sleep 0.25
done

wait
