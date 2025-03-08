#!/usr/bin/env python3

from common import *
import urllib3
urllib3.disable_warnings()
import requests

async def run(username: str, password: str) -> str | None:
    test = CTTest()
    try:
        await test.connect()

        resp: dict = await test.login(username, password)
        tmptoken = resp['id']

        get_uri = f"https://{host}:{port}/set-session?token={tmptoken}"

        print(f"GET {get_uri} ... ", end='')

        get = requests.get(get_uri, verify=False)

        print(get.status_code, end=' ')
        print(get.reason)
        print(get.text)

        cookies = get.cookies.get_dict()
        session_uuid = cookies['session_id']

        await test.close()

        print(f"Session UUID: {session_uuid}")
        return session_uuid
    except Exception as e:
        await test.close()
        raise e

if __name__ == '__main__':
    if len(sys.argv) != 3:
        print(f"Usage: {sys.argv[0]} USERNAME PASSWORD")
        sys.exit(-1)
    asyncio.run(run(sys.argv[1], sys.argv[2]))
