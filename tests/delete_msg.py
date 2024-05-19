#!/usr/bin/env python3

# Simple test to verify if the delete message functionality works correctly.
# A message can be deleted only if it is your own message or if you are the group owner.

import sys
import asyncio
import websockets
import json
import ssl
import re

do_session = True
host = "127.0.0.1"
port = "8080"
uri = f"wss://{host}:{port}"

ssl_context = ssl.SSLContext(ssl.PROTOCOL_TLS_CLIENT)
ssl_context.check_hostname = False
ssl_context.verify_mode = ssl.CERT_NONE;

def repl_func(match: re.Match):
    return " ".join(match.group().split())

async def main(username: str, password: str, msg_id: int) -> int:
    login_packet = {
        "cmd": "login",
        "username": username,
        "password": password,
        "session": do_session
    }
    user_info_packet = {
        "cmd": "client_user_info"
    }
    delete_msg_packet = {
        "cmd": "delete_msg",
        "msg_id": msg_id
    }
    async with websockets.connect(uri, ssl=ssl_context) as ws:
        async def recv_print(oneline_array=False) -> dict:
            packet = json.loads(await ws.recv())
            json_str = json.dumps(packet, indent=4)
            if oneline_array:
                json_str = re.sub(r"(?<=\[)[^\[\]]+(?=])", repl_func, json_str)
            print(f"Recv from server {uri}:\n{json_str}\n")
            return packet

        # Send login command
        await ws.send(str(login_packet))
        recv_packet = await recv_print()
        if recv_packet["cmd"] == "error":
            return -1

        # Send get user info about us
        await ws.send(str(user_info_packet)) 
        recv_packet = await recv_print()
        if recv_packet["cmd"] == "error":
            return -1

        await ws.send(str(delete_msg_packet))
        while True:
            recv_packet = await recv_print()
            if (recv_packet["cmd"] in ["error", "delete_msg"]):
                break

    return 0

if __name__ == '__main__':
    if len(sys.argv) == 4:
        username = sys.argv[1]
        password = sys.argv[2]
        msg_id = int(sys.argv[3])
        ret = asyncio.run(main(username, password, msg_id))
        sys.exit(ret)
    print("Need username, password and msg_id arguments")
    sys.exit(-1)
