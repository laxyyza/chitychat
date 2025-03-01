#!/usr/bin/env python3

import requests

url = "https://localhost:8080/something"
data = {"test": "data"}

resp = requests.post(url, json=data, verify=False)

print("Resp: ", resp)
print(resp.status_code)
print(resp.text)
