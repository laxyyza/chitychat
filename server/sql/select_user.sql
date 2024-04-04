SELECT * FROM Users 
WHERE user_id = $1::int OR username = LOWER($2::varchar(50));
