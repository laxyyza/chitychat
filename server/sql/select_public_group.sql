SELECT g.*
FROM Groups g
JOIN GroupMembers gm ON gm.group_id = g.group_id
WHERE g.public = true AND gm.user_id != $1::int;
-- LIMIT $2::int OFFSET $3::int;
