INSERT INTO RememberTokenUsage(
    token_id, 
    user_agent, 
    ip_address
)
VALUES (
    $1::int,
    $2::text,
    $3::inet
);
