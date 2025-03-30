#!/usr/bin/env python3

from common import *
import urllib3
urllib3.disable_warnings()
import requests
import time

def run(session_id: str, user_id: int, action: str) -> None:
    start_time = time.time()
    resp = requests.post(f"https://{host}:{port}/api/friends/requests", cookies={"session_id": session_id}, verify=False, 
                         json={"user_id": user_id, "action": action})
    end_time = time.time()

    elapsed_time = (end_time - start_time) * 1000

    print(resp.status_code)
    print(resp.text)

    print(f"Request time: {elapsed_time:.2f} ms")

if __name__ == '__main__':
    if (len(sys.argv) != 4):
        print(f"Usage: {sys.argv[0]} [SESSION_UUID] [USER_ID] [ACTION 'accept' | 'block']")
        sys.exit(-1)
    run(sys.argv[1], int(sys.argv[2]), sys.argv[3])
