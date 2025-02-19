#!/usr/bin/python3 

import asyncio
import sys

import register
import session_login
import user_info
import user_groups

username = "test3"
displayname = "Test"
password = "test_pass"

def good(msg: str) -> None:
    print(f"GOOD: {msg}.")

def info(msg: str) -> None:
    print(f"INFO: {msg}")

def bad(msg: str) -> None:
    print(f"BAD: {msg}.")

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

async def do_tests() -> None:
    # Register 
    session: dict = await test_register()

    # Login
    await test_session_login(session)

    # Get user info.
    await test_user_info(session)

    # Create a private and public group.

    # Get user groups.

    # Send a messages in that group

    # Delete a message in that group.

    # Delete group.
 
async def main() -> int:
    try:
        await do_tests()
    except Exception as e:
        print("Exception:", e)
        return -1

    return 0

if __name__ == '__main__':
    if len(sys.argv) == 2:
        username = sys.argv[1]
    ret = asyncio.run(main())
    sys.exit(ret)