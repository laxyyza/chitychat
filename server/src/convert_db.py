#!/usr/bin/env python3

import sqlite3
import hashlib
import os

conn = sqlite3.connect("server/chitychat.db")
cursor = conn.cursor()

def exec_sql_file(file: str):
    with open(file, 'r') as f:
        sql_commands = f.read()
    
    cursor.executescript(sql_commands)
    conn.commit()

def hash_password(password: str, salt=None):
    if salt is None:
        salt = os.urandom(16)
    
    salted_password = password.encode() + salt
    hashed_password = hashlib.sha512(salted_password).digest()

    return hashed_password, salt

def list_table(table_name):
    cursor.execute(f"SELECT * FROM {table_name}")
    rows = cursor.fetchall()

    for row in rows:
        print(row)

# cursor.execute("DROP TABLE NewUsers")

exec_sql_file("server/sql/schema.sql")

cursor.execute("PRAGMA forgein_keys = OFF;")

def convert():
    cursor.execute(
    """
    INSERT INTO NewUsers(user_id, username, displayname, bio, created_at, pfp_name, hash, salt)
    SELECT user_id, username, displayname, bio, created_at, pfp_name, 0x0, 0x0
    FROM Users;
    """)

    cursor.execute(f"SELECT * FROM Users")
    user_rows = cursor.fetchall()
    for user_row in user_rows:
        password = user_row[4]
        user_id = user_row[0]

        hash, salt = hash_password(password)

        cursor.execute(
            """ UPDATE NewUsers
            SET hash = ?, salt = ? WHERE user_id = ?;
            """, (hash, salt, user_id))
    
    
convert()

list_table("NewUsers")
# print("==================")
list_table("Users")
    
conn.commit()
conn.close()
