INSERT INTO RememberTokens(user_id, token_hash, expires_at)
VALUES (
    $1::int,
    $2::bytea,
    now() + $3::interval
)
RETURNING expires_at;
