SELECT g.*, (
    SELECT json_agg(gm2.user_id)
    FROM GroupMembers gm2
    WHERE gm2.group_id = g.group_id
) AS member_ids,
tc.last_message
FROM Groups g 
JOIN GroupMembers gm ON g.group_id = gm.group_id 
JOIN TextChannels tc ON g.channel_id = tc.channel_id
WHERE gm.user_id = $1::int;
