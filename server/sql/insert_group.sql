INSERT INTO Groups (name, "desc", owner_id)
VALUES ($1::VARCHAR(50), $2::TEXT, $3::int)
RETURNING group_id;

-- SELECT last_insert_rowid();