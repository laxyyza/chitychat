INSERT INTO Messages(user_id, channel_id, content)
VALUES ($1::int, $2::int, $3::text)
RETURNING msg_id, timestamp;
