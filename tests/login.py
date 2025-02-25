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

async def main(username, displayname, password) -> int:
    try:
        await run(username, displayname, password)
    except Exception as e:
        return -1
    return 0

if __name__ == '__main__':
    if len(sys.argv) == 4:
        username = sys.argv[1]
        displayname = sys.argv[2]
        password = sys.argv[3]
        ret = asyncio.run(main(username, displayname, password))
        sys.exit(ret)
    print("Need username, displayname and password arguments")
    sys.exit(-1)
