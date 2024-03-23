SELECT * FROM Users 
WHERE user_id = $1::INT OR username = LOWER($2::varchar(50))