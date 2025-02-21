import asyncio
import sys
import websockets
import json
import ssl
import os
import pprint

class CTTest:
    def __init__(self):
        self.do_session: bool = os.getenv("CT_DO_SESSION", "true").lower() in ("true", 1)
        self.host: str = os.getenv("CT_HOST", "127.0.0.1")
        self.port: str = os.getenv("CT_PORT", "8080")
        self.uri: str = os.getenv("CT_URI", f"wss://{self.host}:{self.port}")

        self.ssl_context = ssl.SSLContext(ssl.PROTOCOL_TLS_CLIENT)
        self.ssl_context.check_hostname = False
        self.ssl_context.verify_mode = ssl.CERT_NONE
    
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

    async def connect(self) -> int:
        self.ws = await websockets.connect(self.uri, ssl=self.ssl_context)

    async def close(self) -> None:
        await self.ws.close()