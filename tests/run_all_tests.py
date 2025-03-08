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
import login
import get_session
import requests
import time
from common import *

username = "test1"
displayname = "Test"
password = "test_pass"

def get_req() -> None:
    url = f"https://{host}:{port}"
    print("HTTP GET", url, "...")
    resp = requests.get(url, verify=False)
    
    print(resp.status_code)
    print(resp.text)

"""
Register a new account. 
Then retry registering account with the same username.
"""
async def test_register() -> None:
    try:
        await register.run(username, displayname, password)
    except Exception:
        await login.run(username, password)

    try:
        await register.run(username, displayname, password)
    except Exception:
        good("Got exception when regisitering again")
    else:
        raise RuntimeError("Managed to register twice?")

"""
Login with session ID
Then login with invalid session ID zero.
"""
async def test_session_login(session: str) -> None:
    await session_login.run(session)

    try:
        # Try invalid session ID 0
        session = "0" 
        await session_login.run(session)
    except Exception:
        good(f"Got error when sending invalid session ID")
    else:
        raise RecursionError("Session ID zero worked?")

async def test_user_info(session: str) -> None:
    user: dict = await user_info.run(session)
    if user["username"] != username:
        bad("user_info username is different!")
    if user["displayname"] != displayname:
        bad("user_info dispalyname is different!")
    
    groups: dict = await user_groups.run(session)
    info(f"User:{user['user_id']} is in {len(groups['groups'])} groups")

async def test_create_group(session: str) -> dict:
    return await create_group.run(session, "Group Name", False)

async def test_send_msg(session: str, groups: dict) -> dict:
    group: dict = groups['groups'][0]

    return await send_msg.run(session, group, [
        "This message will NOT be deleted :3",
        "This message WILL be deleted ;(",
    ])

async def test_delete_msg(session: str, msg) -> None:
    await delete_msg.run(session, msg)

def get_test() -> None:
    exception = None
    for _ in range(3):
        try:
            get_req()
            exception = None
            break
        except Exception as e:
            time.sleep(1)
            exception = e
    if exception:
        raise exception

async def do_tests() -> None:
    get_test()

    # Register 
    await test_register()

    # Get session 
    session_uuid: str = await get_session.run(username, password)

    # Login
    await test_session_login(session_uuid)

    # Get user info.
    await test_user_info(session_uuid)

    # Create a private and public group.
    groups: dict = await test_create_group(session_uuid)

    # Send a messages in that group
    msg: dict = await test_send_msg(session_uuid, groups)

    # Delete a message in that group.
    await test_delete_msg(session_uuid, msg)

    # Group Access
    await group_access.run()
 
async def main() -> None:
    await do_tests()

if __name__ == '__main__':
    if len(sys.argv) == 2:
        username = sys.argv[1]
    asyncio.run(main())
    sys.exit(0)
