SELECT dm.user1_id as user_id, tc.last_message
FROM DirectMessages dm
JOIN TextChannels tc ON dm.channel_id = tc.channel_id
WHERE dm.user2_id = $1::int

UNION 

SELECT dm.user2_id as user_id, tc.last_message
FROM DirectMessages dm
JOIN TextChannels tc ON dm.channel_id = tc.channel_id
WHERE dm.user1_id = $1::int;
