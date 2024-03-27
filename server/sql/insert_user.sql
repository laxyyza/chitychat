INSERT INTO Users (username, displayname, bio, hash, salt)
VALUES(
    LOWER($1::VARCHAR(50)), 
    $2::VARCHAR(50), 
    $3::TEXT, 
    $4::BYTEA, 
    $5::BYTEA
);
