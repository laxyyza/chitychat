#!/usr/bin/env python3

from common import *
import urllib3
urllib3.disable_warnings()
import requests
import time

def run(session_id: str) -> None:
    start_time = time.time()
    resp = requests.get(f"https://{host}:{port}/api/friends", cookies={"session_id": session_id}, verify=False)
    end_time = time.time()

    elapsed_time = (end_time - start_time) * 1000

    print(resp.status_code)
    print(resp.text)

    print(f"Request time: {elapsed_time:.2f} ms")

if __name__ == '__main__':
    if (len(sys.argv) != 2):
        print(f"Usage: {sys.argv[0]} [SESSION_UUID]")
        sys.exit(-1)
    run(sys.argv[1])
