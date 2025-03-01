#!/usr/bin/env python3

import sys
import signal
import asyncio
import random
from faker import Faker
from bot_api import *

default_host = "localhost"
default_port = 8080

# Just testing bots, so this doesn't matter
default_secret = "1234"

selected_group: Group = None
fake = Faker()

async def send_group_msg(bot: Bot, loops: int) -> None:
    if selected_group == None:
        return
    text: str = fake.text()
    await bot.send_msg(selected_group, text)

async def get_public_groups_to_join(bot: Bot, loops: int) -> None:
    await bot.get_public_groups()

async def select_random_group(bot: Bot, loops: int) -> None:
    global selected_group
    _, selected_group = random.choice(list(bot.groups.items())) 

async def create_group(bot: Bot, loops: int) -> None:
    await bot.create_new_group(
        name=bot.username + "'s " + "group" + str(loops),
        public=True
    )

random_tasks = [
    send_group_msg,
    get_public_groups_to_join,
    select_random_group,
    create_group
]
tasks_weight = [
    500,
    40,
    40,
    1
]

@cmd
async def session(bot: Bot, packet: dict) -> None:
    bot.session_id = packet["id"]
    bot.logged_in = True
    print("Logged in with session:", bot.session_id)

@cmd
async def client_user_info(bot: Bot, packet: dict) -> None:
    user = packet_to_user(packet)
    user.print()
    bot.users[user.id] = user
    bot.user = user
    bot.got_client_info = True

@cmd
async def client_groups(bot: Bot, packet: dict) -> None:
    global selected_group
    groups: list[dict] = packet["groups"]

    i = 0
    for group_json in groups:
        group = packet_to_group(group_json)
        await bot.get_group_members(group)
        group.print()
        bot.groups[group.id]= group
        if i == 0:
            selected_group = group
        i += 1

    bot.got_client_groups = True

@cmd
async def get_all_groups(bot: Bot, packet: dict) -> None:
    groups_json = packet["groups"]
    group_to_join: dict = None

    if len(groups_json) == 0:
        if selected_group == None:
            await create_group(bot, 0)
        return

    group_to_join = random.choice(groups_json)
    await bot.join_group(group_to_join["group_id"])
    
@cmd
async def rtusm(bot: Bot, packet: dict) -> None:
    user_id: int|None = packet.get("user_id")
    if user_id is not None:
        bot.users[user_id].set_status(packet["status"])

@cmd
async def error(bot: Bot, packet: dict) -> None:
    sys.stderr.write(f"ERROR: '{packet["error_msg"]}' from: {packet["from"]}")

@cmd
async def group_msg(bot: Bot, packet: dict) -> None:
    print(f"Message: {packet["content"]}")

@cmd
async def get_member_ids(bot: Bot, packet: dict) -> None:
    await bot.send("get_user",
        user_ids = packet["member_ids"]
    )

async def connected(bot: Bot) -> None:
   global selected_group

   if len(bot.groups) == 0:
        await get_public_groups_to_join(bot, 0)
   else:
        _, selected_group = random.choice(list(bot.groups.items()))

async def tick(bot: Bot, inter: int) -> None:
    task = random.choices(random_tasks, tasks_weight)[0]
    await task(bot, inter)
    await asyncio.sleep(random.randint(1, 20))

if __name__ == "__main__":
    bot = Bot(
        host=default_host,
        port=default_port,
        username='bot' + sys.argv[1],
        displayname='Bot ' + sys.argv[1],
        secret=default_secret
    );
    bot.mainloop_callback = tick 
    bot.connected_callback = connected

    def handle_signal(sig, frame):
        bot.running = False
        bot.listen_task.cancel()

    signal.signal(signal.SIGINT, handle_signal)
    asyncio.run(bot.run())
