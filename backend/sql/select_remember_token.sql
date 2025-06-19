SELECT token_id, user_id FROM RememberTokens
WHERE token_hash = $1::bytea AND expires_at > now();
