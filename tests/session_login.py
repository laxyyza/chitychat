#!/usr/bin/env python3

# Simple test to see if logging in with session works or not.

from common import *

async def run(session_uuid: str) -> None:
    test = CTTest(session_uuid)
    try:
        await test.connect()

        await test.client_user_info()

        await test.close()
    except Exception as e:
        await test.close()
        raise e
