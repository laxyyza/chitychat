PRAGMA foreign_keys = ON;

-- flags column in tables will be future bit-field options/settings

CREATE TABLE IF NOT EXISTS UserFiles(
    hash        TEXT PRIMARY KEY UNIQUE,
    name        VARCHAR(128) NOT NULL,
    size        INTEGER NOT NULL,
    mime_type   TEXT NOT NULL,
    flags       INTEGER DEFAULT 0,
    ref_count   INTEGER DEFAULT 1
);

CREATE TABLE IF NOT EXISTS Users(
    user_id         INTEGER PRIMARY KEY AUTOINCREMENT UNIQUE,
    username        VARCHAR(50) NOT NULL UNIQUE,
    displayname     VARCHAR(50) NOT NULL,
    bio             TEXT,
    hash            BLOB NOT NULL,
    salt            BLOB NOT NULL,
    flags           INTEGER DEFAULT 0,
    created_at      TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    pfp             TEXT,
    FOREIGN KEY (pfp) REFERENCES UserFiles(hash)
);

CREATE TABLE IF NOT EXISTS Groups(
    group_id        INTEGER PRIMARY KEY AUTOINCREMENT UNIQUE,
    owner_id        INTEGER,
    name            VARCHAR(50) NOT NULL,
    DESC            TEXT,
    flags           INTEGER NOT NULL,
    created_at      TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (owner_id) REFERENCES Users(user_id)
);

CREATE TABLE IF NOT EXISTS GroupMembers(
    user_id         INTEGER,
    group_id        INTEGER,
    join_date       TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    flags           INTEGER DEFAULT 0,
    PRIMARY KEY (user_id, group_id),
    FOREIGN KEY (user_id) REFERENCES Users(user_id),
    FOREIGN KEY (group_id) REFERENCES Groups(group_id)
);

CREATE TABLE IF NOT EXISTS Messages(
    msg_id          INTEGER PRIMARY KEY AUTOINCREMENT UNIQUE,
    user_id         INTEGER, 
    group_id        INTEGER,
    content         TEXT,
    timestamp       TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    attachments     TEXT DEFAULT NULL,
    flags           INTEGER DEFAULT 0,
    parent_msg_id   INTEGER DEFAULT -1,
    FOREIGN KEY (parent_msg_id) REFERENCES Messages(msg_id),
    FOREIGN KEY (user_id) REFERENCES Users(user_id),
    FOREIGN KEY (group_id) REFERENCES Groups(group_id),
    CONSTRAINT group_member
        FOREIGN KEY (user_id, group_id)
        REFERENCES GroupMembers(user_id, group_id),
    CHECK (parent_msg_id != msg_id)
);


-- Everytime when a group is created, 
-- owner will automatically be a group member.
CREATE TRIGGER IF NOT EXISTS insert_owner_group_member
AFTER INSERT ON Groups 
FOR EACH ROW
BEGIN
    INSERT INTO GroupMembers(user_id, group_id)
    VALUES (NEW.owner_id, NEW.group_id);
END;

CREATE TRIGGER IF NOT EXISTS delete_userfiles_if_ref_is_zero
AFTER UPDATE OF ref_count ON UserFiles
WHEN NEW.ref_count <= 0
BEGIN 
    DELETE FROM UserFiles
    WHERE NEW.ref_count <= 0;
END;

-- CREATE TRIGGER IF NOT EXISTS set_default_pfp_name
-- AFTER INSERT ON Users
-- FOR EACH ROW
-- BEGIN
--     UPDATE Users
--     SET pfp_path = '/img/default.png'
--     WHERE user_id = NEW.user_id;
-- END;

CREATE INDEX IF NOT EXISTS idx_get_group_msgs
ON Messages(group_id, msg_id, timestamp DESC, msg_id DESC);