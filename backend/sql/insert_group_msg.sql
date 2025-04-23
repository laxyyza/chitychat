INSERT INTO Messags(user_id, channel_id, content)
SELECT $1::int, g.channel_id, $3::text
FROM Groups g
INNER JOIN GroupMembers gm ON gm.group_id = g.group_id
WHERE g.group_id = $2::int AND gm.user_id = $1::int
RETURNING msg_id, timestamp;
