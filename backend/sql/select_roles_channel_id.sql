SELECT m.roles, c.settings 
FROM hubmembers m
JOIN hubchannels c ON c.hub_id = m.hub_id
WHERE 
    m.hub_id = $1::int AND 
    m.user_id = $2::int AND 
    c.channel_id = $3::int;
