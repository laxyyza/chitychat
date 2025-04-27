SELECT g.*, (
    SELECT json_agg(gm2.user_id)
    FROM GroupMembers gm2
    WHERE gm2.group_id = g.group_id
) AS member_ids
FROM Groups g 
JOIN GroupMembers gm ON g.group_id = gm.group_id
WHERE g.group_id = $1::int AND gm.user_id = $2::int;
