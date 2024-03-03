INSERT INTO Groups (name, desc, owner_id)
VALUES (?, ?, ?);

SELECT last_insert_rowid();