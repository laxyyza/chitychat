#!/usr/bin/env python3

# Simple test to see if creating a new user works or not.

from common import *

test: CTTest = None

async def run(username, password) -> dict:
    test = CTTest()
    try:
        await test.connect()
        session: dict = await test.login(username, password)
        await test.close()

        return session
    except Exception as e:
        await test.close()
        raise e

async def main(username, password) -> int:
    try:
        await run(username, password)
    except Exception as e:
        return -1
    return 0

if __name__ == '__main__':
    if len(sys.argv) == 3:
        username = sys.argv[1]
        password = sys.argv[2]
        ret = asyncio.run(main(username, password))
        sys.exit(ret)
    print("Need username and password arguments")
    sys.exit(-1)
