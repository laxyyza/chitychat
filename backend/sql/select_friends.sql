SELECT target_user_id as friend
FROM Friendships
WHERE source_user_id = $1::int AND status = 'ACCEPTED'

UNION 

SELECT source_user_id as friend
FROM Friendships
WHERE target_user_id = $1::int AND status = 'ACCEPTED';
