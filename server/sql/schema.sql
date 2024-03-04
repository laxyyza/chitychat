PRAGMA foreign_keys = ON;

CREATE TABLE Users(
    user_id         INTEGER PRIMARY KEY AUTOINCREMENT UNIQUE,
    username        VARCHAR(50) NOT NULL UNIQUE,
    displayname     VARCHAR(50) NOT NULL,
    bio             TEXT,
    password        VARCHAR(50) NOT NULL,
    created_at      TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE Groups(
    group_id        INTEGER PRIMARY KEY AUTOINCREMENT UNIQUE,
    owner_id        INTEGER,
    name            VARCHAR(50) NOT NULL,
    DESC            TEXT,
    created_at      TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (owner_id) REFERENCES Users(user_id)
);

CREATE TABLE GroupMembers(
    user_id         INTEGER,
    group_id        INTEGER,
    join_date       TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (user_id, group_id),
    FOREIGN KEY (user_id) REFERENCES Users(user_id),
    FOREIGN KEY (group_id) REFERENCES Groups(group_id)
);

CREATE TABLE Messages(
    msg_id      INTEGER PRIMARY KEY AUTOINCREMENT UNIQUE,
    user_id     INTEGER, 
    group_id    INTEGER,
    timestamp   TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (user_id) REFERENCES Users(user_id),
    FOREIGN KEY (group_id) REFERENCES Groups(group_id),
    CONSTRAINT group_member
        FOREIGN KEY (user_id, group_id)
        REFERENCES GroupMembers(user_id, group_id)
);

CREATE TRIGGER insert_owner_group_member
AFTER INSERT ON Groups 
FOR EACH ROW
BEGIN
    INSERT INTO GroupMembers(user_id, group_id)
    VALUES (NEW.owner_id, NEW.group_id);
END;