INSERT INTO Messages (user_id, group_id, content, attachments)
VALUES (?, ?, ?, ?);

SELECT last_insert_rowid();