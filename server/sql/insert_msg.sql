INSERT INTO Messages (user_id, group_id, content, attachments)
VALUES (
    $1::int, 
    $2::int, 
    $3::text, 
    $4::json
)
RETURNING msg_id;
