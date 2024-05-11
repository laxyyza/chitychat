INSERT INTO Users (username, displayname, hash, salt)
VALUES(
    LOWER($1::varchar(50)), 
    $2::varchar(50), 
    $3::bytea, 
    $4::bytea
)
RETURNING user_id;
