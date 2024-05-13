INSERT INTO GroupMembers(user_id, group_id)
SELECT $1, $2 
FROM Groups g
WHERE g.group_id = $2 AND g.public = true;
