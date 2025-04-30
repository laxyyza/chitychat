DELETE FROM TextChannels tc 
USING Groups g 
WHERE g.channel_id = tc.channel_id AND g.group_id = $1::int AND g.owner_id = $2::int;
