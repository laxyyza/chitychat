SELECT json_agg(msg)
FROM (
    SELECT m.* FROM Messages m
    JOIN Groups g ON g.group_id = $1::int
    JOIN GroupMembers gm ON g.group_id = gm.group_id
    WHERE m.channel_id = g.channel_id AND gm.user_id = $2::int
    ORDER BY msg_id DESC
    LIMIT $3::int OFFSET $4::int
) msg;
