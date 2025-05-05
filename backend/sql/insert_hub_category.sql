INSERT INTO HubCategories(hub_id, name, position)
SELECT hm.hub_id, $2::text, $3::int 
FROM HubMembers hm 
WHERE hm.hub_id = $1::int AND hm.user_id = $4::int 
RETURNING category_id;
