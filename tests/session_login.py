#!/usr/bin/env python3

# Simple test to see if logging in with session works or not.

from common import *

test: CTTest = None

async def run(session: dict) -> None:
    test = CTTest()
    try:
        await test.connect()
        await test.request_wait("session", session)
        await test.close()
    except Exception as e:
        await test.close()
        raise e