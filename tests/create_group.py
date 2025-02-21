#!/usr/bin/env python3

# Simple test to see if creating group works or not.

from common import *

test: CTTest = None

async def run(session: dict, group_name: str, public: bool) -> dict:
    test = CTTest()
    try:
        await test.connect()
        await test.request_wait("session", session, print_packet=False)

        current_groups: dict = await test.request_wait("client_groups", {"cmd": "client_groups"})

        new_groups: dict = await test.request_wait("client_groups", {
            "cmd": "group_create",
            "name": group_name,
            "public": public
        })
        print(f"Went from {len(current_groups['groups'])} -> {len(new_groups['groups'])} groups.")

        await test.close()

        return new_groups
    except Exception as e:
        await test.close()
        raise e