#!/usr/bin/python3

import os

name = os.getenv("SOME_NAME", "default_name")

print("Name: ", name)

os.environ["SOME_NAME"] = "COOL NAME"

name = os.getenv("SOME_NAME", "default_name")

print("Name: ", name)