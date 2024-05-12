#!/usr/bin/env python3

# Simple test to see if logging in as user, and getting user groups work or not

import sys
import asyncio
import websockets
import json
import ssl

do_session = True
host = "127.0.0.1"
port = "8080"
uri = f"wss://{host}:{port}"

ssl_context = ssl.SSLContext(ssl.PROTOCOL_TLS_CLIENT)
ssl_context.check_hostname = False
ssl_context.verify_mode = ssl.CERT_NONE;

async def main(username: str, password: str) -> int:
    login_packet = {
        "cmd": "login",
        "username": username,
        "password": password,
        "session": do_session
    }
    user_info_packet = {
        "cmd": "client_user_info"
    }
    user_groups_packet = {
        "cmd": "client_groups"
    }
    async with websockets.connect(uri, ssl=ssl_context) as ws:
        async def recv_print() -> dict:
            packet = json.loads(await ws.recv())
            print(f"Recv from server {uri}:\n{json.dumps(packet, indent=4)}\n")
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
        
        # Send get all the groups we are in
        await ws.send(str(user_groups_packet))
        recv_packet = await recv_print()
        if recv_packet["cmd"] == "error":
            return -1
    return 0

if __name__ == '__main__':
    if len(sys.argv) == 3:
        username = sys.argv[1]
        password = sys.argv[2]
        ret = asyncio.run(main(username, password))
        sys.exit(ret)
    print("Need username and password arguments")
    sys.exit(-1)
