INSERT INTO GroupMembers (user_id, group_id)
SELECT unnest($1::int[]), g.group_id
FROM Groups g 
WHERE g.group_id = $2::int AND g.owner_id = $3::int
ON CONFLICT DO NOTHING;
