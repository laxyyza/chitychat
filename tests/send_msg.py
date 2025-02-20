#!/usr/bin/env python3

# Simple test to see if sending a message in group works or not.

from common import *

test: CTTest = None

async def run(session: dict, group: dict, msgs: list[str]) -> dict:
    test = CTTest()
    try:
        await test.connect()
        await test.request_wait("session", session)

        group_id: int = group["group_id"]
        group_msg: dict = None

        for msg in msgs:
            group_msg = await test.request_wait("group_msg", {
                "cmd": "group_msg",
                "group_id": group_id,
                "content": msg,
                "attachments": []
            })
            print(f"User:{group_msg['user_id']}: '{group_msg['content']}', {group_msg['timestamp']}")

        await test.close()
        return group_msg
    except Exception as e:
        await test.close()
        raise e