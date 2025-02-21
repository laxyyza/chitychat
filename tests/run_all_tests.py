#!/usr/bin/python3 

import asyncio
import sys

import register
import session_login
import user_info
import user_groups
import create_group
import send_msg
import delete_msg
import group_access
from common import *

username = "test3"
displayname = "Test"
password = "test_pass"

"""
Register a new account. 
Then retry registering account with the same username.
"""
async def test_register() -> dict:
    session: dict = await register.run(username, displayname, password)

    try:
        await register.run(username, displayname, password)
    except Exception:
        good("Got exception when regisitering again")
    else:
        raise RuntimeError("Managed to register twice?")

    return session

"""
Login with session ID
Then login with invalid session ID zero.
"""
async def test_session_login(session: dict) -> None:
    await session_login.run(session)

    real_id = session["id"]

    try:
        # Try invalid session ID 0
        session["id"] = 0
        await session_login.run(session)
    except Exception:
        good("Got error when sending invalid session ID")
        session["id"] = real_id
    else:
        raise RecursionError("Session ID zero worked?")

async def test_user_info(session: dict) -> None:
    user: dict = await user_info.run(session)
    if user["username"] != username:
        bad("user_info username is different!")
    if user["displayname"] != displayname:
        bad("user_info dispalyname is different!")
    
    groups: dict = await user_groups.run(session)
    info(f"User:{user['user_id']} is in {len(groups['groups'])} groups")

async def test_create_group(session: dict) -> dict:
    return await create_group.run(session, "Group Name", False)

async def test_send_msg(session: dict, groups: dict) -> dict:
    group: dict = groups['groups'][0]

    return await send_msg.run(session, group, [
        "This message will NOT be deleted :3",
        "This message WILL be deleted ;(",
    ])

async def test_delete_msg(session, msg) -> None:
    await delete_msg.run(session, msg)

async def do_tests() -> None:
    # Register 
    session: dict = await test_register()

    # Login
    await test_session_login(session)

    # Get user info.
    await test_user_info(session)

    # Create a private and public group.
    groups: dict = await test_create_group(session)

    # Send a messages in that group
    msg: dict = await test_send_msg(session, groups)

    # Delete a message in that group.
    await test_delete_msg(session, msg)

    # Group Access
    await group_access.run()
 
async def main() -> None:
    await do_tests()

if __name__ == '__main__':
    if len(sys.argv) == 2:
        username = sys.argv[1]
    asyncio.run(main())
    sys.exit(0)