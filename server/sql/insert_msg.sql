INSERT INTO Messages (user_id, group_id, content, attachments)
VALUES (
    $1::int, 
    $2::int, 
    $3::TEXT, 
    $4::JSON
)
RETURNING msg_id;
