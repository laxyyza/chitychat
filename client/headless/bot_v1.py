#!/usr/bin/env python

import requests
import websockets
import asyncio
import json
import random
import sys


# for url_file in urls:
#     url = server_url + url_file
#     resp = requests.get(url, verify=False)

#     if resp.status_code == 200:
#         print(resp.text)
#     else:
#         print(f"Error {resp.status_code}")

class Server:
    def __init__(self, host: str, port: int, bot) -> None:
        self.host: str = host
        self.port: int = port
        url: str = f"{self.host}:{self.port}"
        self.http_url: str = f"https://{url}"
        self.ws_url: str = f"wss://{url}"
        self.ws: websockets = None
        self.connected: bool = False
        self.bot = bot 

    async def connect(self) -> bool:
        print("Connecting to " + self.ws_url)
        self.ws = await websockets.connect(self.ws_url, ssl=False, ping_timeout=None)
        # self.ws.recv()
        return True

    async def disconnect(self) -> None:
        if self.connected:
            await self.ws.close()
            self.connected = False
    
    async def run(self) -> None:
        while True:
            packet = await self.recv()
            await self.bot.handle_packet(packet)

    def http_get(self, url: str, print_text: bool=False):
        resp = requests.get(self.http_url + url, verify=False)
        if resp.status_code == 200:
            if print_text:
                print(resp.text)
        else:
            print(f"Error {resp.status_code}")
    
    async def send(self, **kwargs) -> None:
        packet = kwargs
        # packet = json.loads(kwargs)
        # print("Sending", packet)
        await self.ws.send(str(packet))
    
    async def recv(self) -> dict:
        data = json.loads(await self.ws.recv())
        # print("Recv", data)
        return data

class User:
    def __init__(self, id: int, username: str, displayname: str, pfp_name: str) -> None:
        self.id = id
        self.username = username
        self.displayname = displayname
        self.pfp_name = pfp_name

class Group:
    def __init__(self, id: int, name: str) -> None:
        self.id = id
        self.name = name
        self.members: User = []

    def send_msg(self, msg: str) -> None:
        pass

class Message:
    def __init__(self, group: Group, user: User, content: str) -> None:
        self.group = group
        self.user = user
        self.content = content

class Bot:
    def __init__(self, username: str, displayname: str, password: str, server_host: str="chitychat.com", server_port: int=8080, ) -> None:
        self.bot_user = None
        self.current_group = None
        self.username = username
        self.displayname = displayname
        self.password = password
        self.users = {}
        self.groups = {}
        self.server: Server = Server(server_host, server_port, self)
        self.logged_in = False
        self.root_paths = [
            "/",
            "/style.css",
            "/index.js"
        ]
        self.login_paths = [
            "/login",
            "/login/style.css",
            "/login/login.js"
        ]
        self.app_paths = [
            "/app",
            "/app/styles.css",
            "/app/app.js",
            "/app/group.js",
            "/app/server.js",
            "/app/user.js",
        ]
    
    def request_page(self, paths: list[str]) -> None:
        for path in paths:
            self.server.http_get(path)

    async def message(self, msg: Message) -> None:
        pass

    async def get_client_data(self):
        await self.server.send(
            type="client_user_info"
        )
        await self.server.send(
            type="client_groups"
        )

    async def login(self) -> bool:
        await self.server.send(
            type="login", 
            username=self.username, 
            password=self.password
        )
        packet = await self.server.recv()
        if packet['type'] == "session":
            self.session = packet
            await self.get_client_data()
            return True
        else:
            return False

    async def register_account(self) -> bool:
        await self.server.send(
            type="register",
            username=self.username,
            displayname=self.displayname,
            password=self.password
        )
        packet = await self.server.recv()
        if packet['type'] == "session":
            self.session = packet
            await self.get_client_data()
            return True
        else:
            return False
        
    async def handle_packet(self, packet: dict) -> None:
        match packet['type']:
            case "session":
                self.logged_in = True

            case "client_user_info":
                self.bot_user = User(packet['id'], packet['username'], packet['displayname'], packet['pfp_name'])
                self.users[self.bot_user.id] = self.bot_user

            case "client_groups":
                groups: list = packet['groups']
                for group in groups:
                    self.groups[group['id']] = Group(group['id'], group['name'])
                    self.current_group = self.groups[group['id']]
                    members: list[int] = group['members_id']
                    for member_id in members:
                        await self.server.send(
                            type="get_user",
                            id=member_id
                        )

            case "get_user":
                user_id: int = packet['id']
                username: str = packet['username']
                displayname: str = packet['displayname']
                pfp_name: str = packet['pfp_name']

                self.users[user_id] = User(user_id, username, displayname, pfp_name)
                self.server.http_get(pfp_name)

            case "group_msg":

                user_id: int = packet['user_id']
                if user_id == self.bot_user.id:
                    return
                group_id: int = packet['group_id']
                # content: str = packet['content']

                try:
                    user: User = self.users[user_id]
                    group: Group = self.groups[group_id]
                except:
                    await self.server.send(
                        type="get_user",
                        id=user_id
                    )

                # print(f"{user.displayname} on '{group.name}':\n> {content}")
            
            case "get_all_groups":
                if not self.groups:
                    groups: list = packet['groups']
                    group = random.choice(groups)
                    
                    print("Random group", group)
                    await self.server.send(
                        type="join_group",
                        group_id=group['id']
                    )

            case _:
                print("Need to support packet type", packet['type'])
    
    async def get_groups(self) -> None:
        await self.server.send(
            type="get_all_groups"
        )

    async def mainloop(self) -> None:
        print("main loop")
        n: int = 0
        await asyncio.sleep(2)
        while True:
            await asyncio.sleep(1)

            try:
                await self.server.send(
                    type="group_msg",
                    group_id=self.current_group.id,
                    content="Loop " + str(n)
                )
            except: pass
            n += 1
    
    async def mainloop_server(self):
        await self.server.recv()

    async def run(self) -> None:
        await self.server.connect()
        # self.request_page(self.root_paths)
        # await asyncio.sleep(1)
        # self.request_page(self.login_paths)
        if not await self.login():
            await self.register_account()

        await self.get_groups()

        # await self.server.send(
        #     type="login",
        #     username=self.username,
        #     password=self.password
        # )

        recv_task = asyncio.ensure_future(self.server.run())
        bot_task = asyncio.ensure_future(self.mainloop())

        await asyncio.wait([recv_task, bot_task], return_when=asyncio.FIRST_COMPLETED)

        # await asyncio.gather(self.mainloop_server(), self.mainloop())
        # await self.server.run()

        # await self.server.send(
        #     type="client_user_info"
        # )
        # packet = await self.server.recv()

        # await self.server.send(
        #     type="client_groups"
        # )
        # packet = await self.server.recv()
        # if packet['type'] == "error":
        #     await self.join_random_group()

if __name__ == '__main__':
    num = sys.argv[1]
    bot: Bot = Bot(
        username="bot" + str(num), 
        displayname="Bot " + str(num), 
        password="1234"
    )
    asyncio.run(bot.run())
