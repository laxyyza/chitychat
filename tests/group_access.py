#!/usr/bin/env python3

"""
Goal: Verify that only authorized users can interact with a private group, 
while any user can interact with a public group. Additionally, user that 
a private group becomes accessible to User 2 after receiving an invitation from User 1.

Steps:
    1. Create two users.
    2. User 1 creates both a private and a public group.
    3. User 2 tries to join the private group, send messages, and retrieve group information (expect failure due to privacy restrictions).
    4. User 2 tries the same actions with the public group (expect success).
    5. User 1 invites User 2 to the private group.
    6. User 2 accepts the invitation and retries the actions (expect success).

"""

from common import *

async def test_user_permission_denied(user2: CTTest, group_id: int) -> None:
    try:
        await user2.join_group(group_id)
    except Exception as e:
        good("Joining a private group failed")
    else:
        raise RuntimeError("Managed to join a private group?")

    try:
        await user2.send_msg(group_id, "You should not see this message.")
    except Exception as e:
        good("Sending a message failed")
    else:
        raise RuntimeError("Managed to a message?")
    
    try:
        await user2.get_group_msgs(group_id)
    except Exception as e:
        good("Getting group messages failed")
    else:
        raise RuntimeError("Managed to get group messages?")

    try:
        await user2.get_member_ids(group_id)
    except Exception as e:
        good("Getting group members failed")
    else:
        raise RuntimeError("Manged to get group members?")

async def run() -> None:
    user1: CTTest = CTTest()
    user2: CTTest = CTTest()

    try:
        await user1.connect()
        await user2.connect()

        await user1.register("tgau1", "Test Group Access User 1", "test_pass")
        await user2.register("tgau2", "Test Group Access User 2", "test_pass")

        private_group: dict = await user1.create_group("Private Group", False)
        public_group: dict = await user1.create_group("Public Group", True)

        private_group = private_group["groups"][0]
        public_group = public_group["groups"][0]

        private_group_id = private_group['group_id']

        await user1.send_msg(private_group_id, "Super Secret Message 1")
        await user1.send_msg(private_group_id, "Super Secret Message 2")

        await test_user_permission_denied(user2, private_group_id)

        await user1.close()
        await user2.close()
    except Exception as e:
        await user1.close()
        await user2.close()
        raise e
