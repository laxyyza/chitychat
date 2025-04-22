#!/usr/bin/env python3

# Simple test to see if logging in with session works or not.

from common import *
import sys
import asyncio

async def run(session_uuid: str) -> None:
    test = CTTest(session_uuid)
    try:
        await test.connect()

        await test.client_user_info()

        await test.close()
    except Exception as e:
        await test.close()
        raise e

if __name__ == '__main__':
    if len(sys.argv) != 2:
        print(f"Usage: {sys.argv[0]} [SESSION_UUID]")
        sys.exit(-1)
    asyncio.run(run(sys.argv[1]))
