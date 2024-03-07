INSERT INTO Users (username, displayname, bio, hash, salt)
VALUES(LOWER(?), ?, ?, ?, ?);