SELECT user1_id as user_id
FROM DirectMessages
WHERE user2_id = $1::int

UNION 

SELECT user2_id as user_id
FROM DirectMessages
WHERE user1_id = $1::int;