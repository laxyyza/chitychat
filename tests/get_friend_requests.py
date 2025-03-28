#!/usr/bin/env python3

from common import *
import urllib3
urllib3.disable_warnings()
import requests

def run(session_id: str) -> None:
    resp = requests.get(f"https://{host}:{port}/api/friend-requests", cookies={"session_id": session_id}, verify=False)
    print(resp.status_code)
    print(resp.text)

if __name__ == '__main__':
    if (len(sys.argv) != 2):
        print(f"Usage: {sys.argv[0]} [SESSION_UUID]")
        sys.exit(-1)
    run(sys.argv[1])
