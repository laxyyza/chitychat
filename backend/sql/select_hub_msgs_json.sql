-- $1::int HUB_ID 
-- $2::int USER_ID 
-- $3::int CHANNEL_ID 
-- $4::int LIMIT 
-- $5::itn OFFSET

SELECT json_build_object(
    'channel_id', $3::int,
    'hub_id', $1::int,
    'messages', (
        SELECT json_agg(msg)
        FROM (
            SELECT m.msg_id, m.user_id, m.content, m.attachments, m.timestamp
            FROM Messages m
            JOIN HubChannels c ON c.channel_id = m.channel_id
            JOIN HubMembers hm ON c.hub_id = hm.hub_id
            WHERE   hm.hub_id = $1::int
                AND c.channel_id = $3::int
                AND hm.user_id = $2::int
            LIMIT $4::int OFFSET $5::int
        ) msg
    )
);
