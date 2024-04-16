INSERT INTO Users (username, displayname, bio, hash, salt)
VALUES(
    LOWER($1::varchar(50)), 
    $2::varchar(50), 
    $3::text, 
    $4::bytea, 
    $5::bytea
);
