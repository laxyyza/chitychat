SELECT json_agg(msg)
FROM (
    SELECT *
    FROM Messages
    WHERE group_id = $1::int
    ORDER BY msg_id DESC
    LIMIT $2::int OFFSET $3::int
) msg;
