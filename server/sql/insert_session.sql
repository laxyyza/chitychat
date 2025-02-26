INSERT INTO Sessions(user_id)
VALUES (
    $1::int
)
RETURNING session_id;