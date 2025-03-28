#!/usr/bin/env python3

from common import *
import urllib3
urllib3.disable_warnings()
import requests

def run(session_id: str, username: str) -> None:
    resp = requests.post(f"https://{host}:{port}/api/friend-request", cookies={"session_id": session_id}, verify=False, json={"username": username})
    print(resp.status_code)
    print(resp.text)

if __name__ == '__main__':
    if (len(sys.argv) != 3):
        print(f"Usage: {sys.argv[0]} <SESSION_UUID> <USERNAME>")
        sys.exit(-1)
    run(sys.argv[1], sys.argv[2])