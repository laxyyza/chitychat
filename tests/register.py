#!/usr/bin/env python3

# Simple test to see if creating a new user works or not.

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

async def main(username: str, displayname: str, password: str) -> int:
    login_packet: dict = {
        "cmd": "register",
        "username": username,
        "displayname": displayname,
        "password": password,
        "session": do_session
    }
    ret = -1
    async with websockets.connect(uri, ssl=ssl_context) as ws:
        await ws.send(str(login_packet))
        recv_packet = json.loads(await ws.recv())
        if recv_packet["cmd"] == "session":
            ret = 0
        print(f"Recv from server {uri}:\n\t{recv_packet}")
    return ret

if __name__ == '__main__':
    if len(sys.argv) == 4:
        username = sys.argv[1]
        displayname = sys.argv[2]
        password = sys.argv[3]
        ret = asyncio.run(main(username, displayname, password))
        sys.exit(ret)
    print("Need username, displayname and password arguments")
    sys.exit(-1)
