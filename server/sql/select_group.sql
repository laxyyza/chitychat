SELECT DISTINCT g.*
FROM Groups g 
JOIN GroupMembers gm ON g.group_id = gm.group_id
WHERE gm.group_id = $1::int OR gm.user_id = $2::int;

-- SELECT * FROM Groups WHERE group_id = ? OR owner_id = ?