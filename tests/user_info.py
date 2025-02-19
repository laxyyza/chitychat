#!/usr/bin/env python3

# Simple test to see if logging in and getting user info works or not.

from common import *

test: CTTest = None

async def run(session: dict) -> dict:
    test = CTTest()
    try:
        await test.connect()
        await test.request_wait("session", session)

        client_user_info:dict = await test.request_wait("client_user_info", {"cmd": "client_user_info"})

        await test.close()
        return client_user_info 
    except Exception as e:
        await test.close()
        raise e