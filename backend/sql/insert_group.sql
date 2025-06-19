INSERT INTO Groups (name, owner_id, public)
VALUES (
    $1::varchar(50), 
    $2::int,
    $3::boolean
)
RETURNING group_id;
