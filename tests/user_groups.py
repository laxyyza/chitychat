#!/usr/bin/env python3

# Simple test to see if logging in and getting user groups works or not.

from common import *

test: CTTest = None

async def run(session: dict) -> dict:
    global test
    try:
        if test is None:
            test = CTTest()
            await test.connect()
            await test.request_wait("session", session)

        groups: dict = await test.request_wait("client_groups", {"cmd": "client_groups"})

        if session:
            await test.close()
        return groups
    except Exception as e:
        if session:
            await test.close()
        raise e

async def main(username: str, password: str) -> None:
    global test
    test = CTTest()
    try:
        await test.connect()
        await test.login(username, password)

        await run(None)

        await test.close()
    except Exception as e:
        await test.close()
        raise e

if __name__ == '__main__':
    if len(sys.argv) != 3:
        print(f"Usage: {sys.argv[0]} USERNAME PASSWORD")
        sys.exit(-1)
    asyncio.run(main(sys.argv[1], sys.argv[2]))