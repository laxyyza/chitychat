#!/usr/bin/env python3

# Simple test to see if deleting a group works or not AND ONLY if the group owner have permission to delete his group.

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

def is_owner(groups: dict, group_id: int, user_id: int) -> bool:
    group = None
    for g in groups:
        if g["group_id"] == group_id:
            group = g
            break
    if group == None:
        return False
    return group["owner_id"] == user_id

async def main(username: str, password: str, group_id: int) -> int:
    user: dict
    groups: dict
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
    delete_group_packet = {
        "cmd": "delete_group",
        "group_id": group_id
    }
    async with websockets.connect(uri, ssl=ssl_context) as ws:
        async def recv_print(oneline_array=False, print_data=True) -> dict:
            packet = json.loads(await ws.recv())
            json_str = json.dumps(packet, indent=4)
            if print_data or packet["cmd"] == "error":
                if oneline_array:
                    json_str = re.sub(r"(?<=\[)[^\[\]]+(?=])", repl_func, json_str)
                print(f"Recv from server {uri}:\n{json_str}\n")
            return packet

        # Send login command
        await ws.send(str(login_packet))
        recv_packet = await recv_print(print_data=False)
        if recv_packet["cmd"] == "error":
            return -1

        # Send get user info about us
        await ws.send(str(user_info_packet)) 
        recv_packet = await recv_print(print_data=False)
        if recv_packet["cmd"] == "error":
            return -1
        user = recv_packet
        
        # Send get all the groups we are in
        await ws.send(str(user_groups_packet))
        recv_packet = await recv_print(print_data=False)
        if recv_packet["cmd"] == "error":
            return -1
        groups = recv_packet["groups"]

        await ws.send(str(delete_group_packet))
        recv_packet = await recv_print()
        if recv_packet["cmd"] == "error":
            failed_to_delete = True
        else:
            failed_to_delete = False
        
        print(">>> \"", end='')
        if failed_to_delete:
            if is_owner(groups, group_id, user["user_id"]):
                print("Is owner and failed to delete group. Bad, it should've deleted it!", end='')
                ret = -1
            else:
                print("Was not an owner and failed to delete group. Good!", end='')
                ret = 0
        else:
            if is_owner(groups, group_id, user["user_id"]):
                print("Is owner and deleted the group. Good!", end='')
                ret = 0
            else:
                print("Deleted the group and was not the owner. SUPER BAD!!!", end='')
                ret = -1
        print("\" <<<")
        return ret

if __name__ == '__main__':
    if len(sys.argv) == 4:
        username = sys.argv[1]
        password = sys.argv[2]
        group_id = int(sys.argv[3])
        ret = asyncio.run(
            main(username, password, group_id)
        )
        sys.exit(ret)
    print("Need username, password & group_id arguments")
    sys.exit(-1)
