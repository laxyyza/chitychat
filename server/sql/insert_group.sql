INSERT INTO Groups (name, "desc", owner_id, public)
VALUES (
    $1::varchar(50), 
    $2::text, 
    $3::int,
    $4::boolean
)
RETURNING group_id;
