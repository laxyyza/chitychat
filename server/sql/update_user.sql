UPDATE Users
SET 
username = CASE 
    WHEN $1::boolean 
    THEN 
        $2::varchar(50) 
    ELSE 
        username 
    END,

displayname = CASE 
    WHEN $3::boolean 
    THEN 
        $4::varchar(50) 
    ELSE 
        displayname 
    END,

pfp = CASE 
    WHEN $5::boolean 
    THEN 
        $6::text
    ELSE 
        pfp 
    END

WHERE user_id = $7::int;
