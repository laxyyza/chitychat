INSERT INTO LoginAttempts(
    username, 
    successful, 
    user_agent, 
    ip_address
)
VALUES (
    $1::text,
    $2::boolean,
    $3::text,
    $4::inet
);
