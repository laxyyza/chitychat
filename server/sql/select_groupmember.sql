-- Select Group Members as Users
SELECT DISTINCT u.*
FROM Users u 
JOIN GroupMembers gm ON u.user_id = gm.user_id
WHERE gm.group_id = ?;