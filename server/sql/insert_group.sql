INSERT INTO Groups (name, "desc", owner_id, flags)
VALUES (
    $1::VARCHAR(50), 
    $2::TEXT, 
    $3::int,
    $4::int
)
RETURNING group_id;
