#!/usr/bin/env python3

# Simple test to see if deleting a message in group works or not.

from common import *

test: CTTest = None

async def run(session: dict, msg: dict) -> None:
    test = CTTest()
    try:
        await test.connect()
        await test.request_wait("session", session, print_packet=False)

        group_msgs: dict = await test.request_wait("get_group_msgs", {
            "cmd": "get_group_msgs",
            "group_id": msg['group_id'],
            "limit": 10000,
            "offset": 0
        })

        old_msg_count = len(group_msgs["messages"])

        await test.request_wait("delete_msg", {
            "cmd": "delete_msg",
            "msg_id": msg['msg_id']
        })

        group_msgs: dict = await test.request_wait("get_group_msgs", {
            "cmd": "get_group_msgs",
            "group_id": msg['group_id'],
            "limit": 10000,
            "offset": 0
        })
        new_msg_count = len(group_msgs["messages"])

        await test.close()

        print(f"Message count for group:{msg['group_id']}: {old_msg_count} -> {new_msg_count}")

        if new_msg_count == old_msg_count:
            raise RuntimeError("Did not delete message?")
    except Exception as e:
        await test.close()
        raise e