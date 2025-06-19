#!/usr/bin/env python3

# Simple test to see if logging in with session works or not.

from common import *

async def run(session_uuid: str, cmd: str) -> None:
    test = CTTest(session_uuid)
    try:
        await test.connect()
        await test.recv()

        await test.ws.send(cmd)
        await test.recv()

        await test.close()
    except Exception as e:
        await test.close()
        raise e

if __name__ == '__main__':
    if len(sys.argv) != 3:
        print(f"Usagae: {sys.argv[0]} [SESSION_UUID] [JSON]")
        sys.exit(-1)
    asyncio.run(run(sys.argv[1], sys.argv[2]))
