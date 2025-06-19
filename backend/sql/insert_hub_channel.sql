INSERT INTO HubChannels (hub_id, category_id, name, position)
SELECT hm.hub_id, $2::int, $3::text, $4::int
FROM HubMembers hm 
WHERE hm.hub_id = $1::int AND hm.user_id = $5::int
RETURNING *;
