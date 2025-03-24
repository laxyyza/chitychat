SELECT json_agg(g.*)
FROM Groups g
LEFT JOIN GroupMembers gm ON g.group_id = gm.group_id AND gm.user_id = $1::int
WHERE g.public = true AND gm.group_id IS NULL;
-- LIMIT $2::int OFFSET $3::int;
