#!/usr/bin/env python3

# Simple test to see if sending a message in group works or not.

from common import *

test: CTTest = None

async def run(session: str, group: dict, msgs: list[str]) -> dict:
    test = CTTest(session)
    try:
        await test.connect()

        group_id: int = group["group_id"]
        group_msg: dict = None

        for msg in msgs:
            group_msg = await test.send_msg(group_id, msg)
            print(f"User:{group_msg['user_id']}: '{group_msg['content']}', {group_msg['timestamp']}")

        await test.close()
        return group_msg
    except Exception as e:
        await test.close()
        raise e
