SELECT json_agg(msg)
FROM (
    SELECT m.* 
    FROM Messages m
    JOIN HubChannels c ON c.channel_id = m.channel_id
    JOIN HubMembers hm ON c.hub_id = hm.hub_id
    WHERE   hm.hub_id = $1::int 
        AND hm.user_id = $2::int 
        AND c.channel_id = $3::int
    ORDER BY m.msg_id DESC
    LIMIT $4::int OFFSET $5::int
) msg;

