INSERT INTO Messages (user_id, group_id, content)
VALUES (?, ?, ?);

SELECT last_insert_rowid();