UPDATE RememberTokens
SET token_hash = $1::bytea,
    last_used_at = now(),
    expires_at = now() + $2::interval
WHERE token_id = $3::int
RETURNING expires_at;
