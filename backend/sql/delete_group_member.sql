DELETE FROM GroupMembers gm 
USING Groups g 
WHERE g.group_id = gm.group_id AND gm.group_id = $1::int AND gm.user_id = $2::int AND g.owner_id != $2::int;
