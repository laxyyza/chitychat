SELECT g.*
FROM Groups g 
JOIN GroupMembers gm ON g.group_id = gm.group_id
WHERE gm.user_id = $1::int;
