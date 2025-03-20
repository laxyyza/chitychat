SELECT m.*
FROM Messages m
WHERE m.group_id = $1::int OR m.msg_id = $2::int
ORDER BY m.timestamp DESC, m.msg_id DESC
LIMIT $3::int OFFSET $4::int;
