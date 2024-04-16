INSERT INTO Groups (name, "desc", owner_id, flags)
VALUES (
    $1::varchar(50), 
    $2::text, 
    $3::int,
    $4::int
)
RETURNING group_id;
