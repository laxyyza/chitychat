SELECT DISTINCT g.*
FROM Groups g 
JOIN GroupMembers gm ON g.group_id = gm.group_id
WHERE gm.group_id = ? OR gm.user_id = ?;

-- SELECT * FROM Groups WHERE group_id = ? OR owner_id = ?