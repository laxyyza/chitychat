-- Get all the connected users
-- "Connected Users" mean all the users you are in a group with.

-- Get the user group IDs
WITH user_group_ids AS (
    SELECT g.group_id
    FROM Groups g
    JOIN GroupMembers gm ON gm.group_id = g.group_id
    WHERE gm.user_id = $1::int
)
-- Select all group members from the group IDs
SELECT DISTINCT u.user_id
FROM Users u 
JOIN GroupMembers gm ON gm.group_id IN (
    SELECT group_id FROM user_group_ids
)
WHERE u.user_id = gm.user_id;
