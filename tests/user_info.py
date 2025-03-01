#!/usr/bin/env python3

# Simple test to see if logging in and getting user info works or not.

from common import *

test: CTTest = None

async def run(session: str) -> dict:
    test = CTTest(session)
    try:
        await test.connect()

        client_user_info:dict = await test.client_user_info()

        await test.close()
        return client_user_info 
    except Exception as e:
        await test.close()
        raise e
