import asyncio
import sys
import websockets
import json
import ssl
import os
import pprint
from packaging import version

def good(msg: str) -> None:
    print(f"GOOD: {msg}.")

def info(msg: str) -> None:
    print(f"INFO: {msg}")

def bad(msg: str) -> None:
    print(f"BAD: {msg}.")


class CTTest:
    def __init__(self, session_uuid = None, do_session: bool=True):
        self.do_session: bool = do_session
        self.host: str = os.getenv("CT_HOST", "127.0.0.1")
        self.port: str = os.getenv("CT_PORT", "8080")
        self.session_uuid = session_uuid
        if session_uuid:
            self.path: str = '/'
        else:
            self.path: str = '/login'
        self.set_uri()

        self.ssl_context = ssl.SSLContext(ssl.PROTOCOL_TLS_CLIENT)
        self.ssl_context.check_hostname = False
        self.ssl_context.verify_mode = ssl.CERT_NONE

    def set_uri(self, path = None) -> None:
        if path:
            self.path = path
        self.uri: str = os.getenv("CT_URI", f"wss://{self.host}:{self.port}{self.path}")
    
    async def request(self, request: dict) -> None:
        print("Sending ", str(request))
        await self.ws.send(str(request))
    
    async def recv(self, print_packet: bool=True) -> dict:
        packet: dict = json.loads(await self.ws.recv())
        json_str = json.dumps(packet, indent=4)
        if print_packet:
            print(f"From {self.uri}:\n{json_str}\n")

        if packet["cmd"] == "error":
            raise RuntimeError(packet)

        return packet

    async def request_wait(self, expected_cmd: str, request: dict, print_packet: bool=True) -> dict:
        await self.request(request)
        recv_packet: dict = await self.recv(print_packet)
        recv_cmd = recv_packet["cmd"]
        if expected_cmd:
            while recv_cmd != expected_cmd:
                recv_packet = await self.recv()
                recv_cmd = recv_packet["cmd"]
        return recv_packet

    async def connect(self, path=None) -> None:
        self.set_uri(path)
        cookie_headers = None
        if self.session_uuid:
            cookie_headers = [('Cookie', f'session_id={self.session_uuid}')]
        print("Connecting to ", self.uri)

        if version.parse(websockets.version.version) >= version.parse("14"):
            self.ws = await websockets.connect(self.uri, 
                                               ssl=self.ssl_context, 
                                               additional_headers=cookie_headers) 
        else:
            self.ws = await websockets.connect(self.uri, 
                                               ssl=self.ssl_context, 
                                               extra_headers=cookie_headers) 


    async def close(self) -> None:
        await self.ws.close()
    
    async def login(self, username: str, password: str) -> dict:
        return await self.request_wait("session", {
            "cmd": "login",
            "username": username,
            "password": password,
            "session": self.do_session
        })

    async def client_user_info(self) -> dict:
        return await self.request_wait("client_user_info", {
            "cmd": "client_user_info",
        })
    
    async def delete_msg(self, msg_id: int) -> dict:
        return await self.request_wait("delete_msg", {
            "cmd": "delete_msg",
            "msg_id": msg_id
        })
    
    async def register(self, username: str, displayname: str, password: str) -> dict:
        return await self.request_wait("session", {
            "cmd": "register",
            "username": username,
            "displayname": displayname,
            "password": password,
            "session": self.do_session
        })
    
    async def create_group(self, name: str, public: bool) -> dict:
        return await self.request_wait("client_groups", {
            "cmd": "group_create",
            "name": name,
            "public": public
        })
    
    async def send_msg(self, group_id: int, content: str) -> dict:
        return await self.request_wait("group_msg", {
            "cmd": "group_msg",
            "group_id": group_id,
            "content": content,
            "attachments": []
        })
    
    async def join_group(self, group_id: int) -> dict:
        return await self.request_wait("client_groups", {
            "cmd": "join_group",
            "group_id": group_id
        })
    
    async def get_group_msgs(self, group_id: int, limit: int = 10000, offset: int = 0) -> dict:
        return await self.request_wait("get_group_msgs", {
            "cmd": "get_group_msgs",
            "group_id": group_id,
            "limit": limit,
            "offset": offset
        })

    async def get_member_ids(self, group_id: int) -> dict:
        return await self.request_wait("get_member_ids", {
            "cmd": "get_member_ids",
            "group_id": group_id
        })
