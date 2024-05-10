import websockets
import ssl
import asyncio
import json

do_session = False
commands = {}

def cmd(func):
    commands[func.__name__] = func

class User:
    def __init__(self, id: int, 
                 username: str, 
                 displayname: str, 
                 bio: str, 
                 created_at: str, 
                 pfp_name: str, 
                 status: str) -> None:
        self.id = id
        self.username = username
        self.displayname = displayname
        self.bio = bio
        self.created_at = created_at
        self.pfp_name = pfp_name
        self.status = status

    def set_status(self, status: str) -> None:
        self.status = status

    def print(self) -> None:
        print(f"User id:{self.id}, username:'{self.username}', displayname:'{self.displayname}', status:{self.status}")

def packet_to_user(packet: dict) -> User:
    return User(
        packet["user_id"],
        packet["username"],
        packet["displayname"],
        packet["bio"],
        packet["created_at"],
        packet["pfp_name"],
        packet["status"],
    )

class Message:
    def __init__(self, 
                 msg_id: int,
                 group_id: int,
                 user_id: int,
                 content: str,
                 attachments: list,
                 timestamp: str) -> None:
        self.msg_id = msg_id
        self.group_id = group_id
        self.group: Group = None
        self.user_id = user_id
        self.user: User = None
        self.content = content
        self.attachments = attachments
        self.timestamp = timestamp
    
    def print(self) -> None:
        pass

def packet_to_msg(packet: dict) -> Message:
    return Message(
        packet["msg_id"],
        packet["group_id"],
        packet["user_id"],
        packet["content"],
        packet["attachments"],
        packet["timestamp"],
    )

class Group:
    def __init__(self, 
                 id: int, 
                 owner_id: int, 
                 name: str, 
                 public: bool,
                 members_id: list[int]) -> None:
        self.id = id
        self.owner_id = owner_id
        self.name = name
        self.public = public
        self.members_id = members_id
        self.members: dict[int, User] = {}

    def try_resolve_members(self, cached_members: dict[int, User]) -> None:
        for member_id in self.members_id:
            member = cached_members.get(member_id)
            if member:
                self.members[member.id] = member

    def print(self) -> None:
        print(f"Group id:{self.id},\towner_id:{self.owner_id}, name:'{self.name}',\t\tpublic:{self.public}, Members_ID:{self.members_id}\
                \n\tMembers:{self.members}")

def packet_to_group(packet: dict) -> Group:
    return Group(
        packet["group_id"],
        packet["owner_id"],
        packet["name"],
        packet["public"],
        packet["members_id"],
    )

class Bot:
    def __init__(self,
                 host: str,
                 port: int,
                 username: str, 
                 displayname: str, 
                 secret: str) -> None:
        self.host = host
        self.port = port
        self.username = username
        self.displayname = displayname
        self.secret = secret
        self.users = {}
        self.groups = {}
        self.running = True
        self.logged_in = False
        self.session_id = 0
        self.user = None
        self.mainloop_callback = None
        self.connected_callback = None
        self.got_client_info = False
        self.got_client_groups = False
        self.uri: str = "wss://" + self.host + ":" + str(self.port)

    async def send(self, cmd, **kwargs):
        kwargs["cmd"] = cmd
        print("SENDING:", str(kwargs))
        return await self.ws.send(str(kwargs))
    
    async def handle_packet(self, packet: dict):
            callback = commands.get(packet["cmd"])
            if callback is not None:
                await callback(self, packet)
            else:
                print("No callback for packet cmd:", packet["cmd"])

    async def listen(self) -> None:
        while self.running:
            packet = await self.recv()
            await self.handle_packet(packet)
    
    async def recv(self) -> dict:
        ret = json.loads(await self.ws.recv())
        print("RECV", ret)
        return ret
    
    async def connect(self):
        ssl_context = ssl.SSLContext(ssl.PROTOCOL_TLS_CLIENT)
        ssl_context.check_hostname = False
        ssl_context.verify_mode = ssl.CERT_NONE

        self.ws = await websockets.connect(self.uri, ssl=ssl_context, ping_interval=None) 
    
    async def register(self) -> bool:
        await self.send("register",
            username = self.username,
            displayname = self.displayname,
            password = self.secret,
            session = do_session
        )

        packet = await self.recv()
        if packet["cmd"] == "error":
            return False
        
        await self.handle_packet(packet)

        return True

    async def login(self) -> bool:
        await self.send("login",
            username = self.username,
            password = self.secret,
            session = do_session 
        )
        packet = await self.recv()
        if packet["cmd"] == "error":
            return False
        
        await self.handle_packet(packet)

        return True

    async def get_info(self) -> None:
        await self.send("client_user_info")
        await self.send("client_groups")

        while self.got_client_info == False or self.got_client_groups == False:
            await asyncio.sleep(0.5)

    async def run(self) -> None:
        await self.connect()
        if await self.login() == False and await self.register() == False:
            return
        self.listen_task = asyncio.create_task(self.listen())

        await self.get_info()
        await self.connected_callback(self)

        i: int = 0
        while self.running:
            await self.mainloop_callback(self, i)
            i += 1

        await self.ws.close()

    async def send_msg(self, group: Group, content: str, attachments: list=[]) -> None:
        await self.send("group_msg",
            group_id = group.id,
            content = content,
            attachments = attachments
        )
    
    async def create_new_group(self, name: str, public: bool):
        await self.send("group_create", 
            name = name,
            public = public
        )
    
    async def get_public_groups(self):
        await self.send("get_all_groups")
    
    async def join_group(self, group_id: int):
        await self.send("join_group",
            group_id = group_id
        )
