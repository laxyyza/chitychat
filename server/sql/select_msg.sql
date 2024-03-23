SELECT m.*
FROM Messages m
JOIN Groups g ON m.group_id = g.group_id
WHERE m.group_id = $1::int OR m.msg_id = $2::int
ORDER BY m.timestamp DESC, m.msg_id DESC
LIMIT $3::int OFFSET $4::int;