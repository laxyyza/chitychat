SET CONSTRAINTS ALL DEFERRED;

CREATE EXTENSTION IF NOT EXISTS pgcrypto;

CREATE TABLE IF NOT EXISTS UserFiles(
    hash            TEXT PRIMARY KEY UNIQUE,
    size            BIGINT NOT NULL,
    mime_type       TEXT NOT NULL,
    flags           INTEGER DEFAULT 0,
    ref_count       INTEGER DEFAULT 1
);

CREATE TABLE IF NOT EXISTS Users(
    user_id         SERIAL PRIMARY KEY,
    username        VARCHAR(50) NOT NULL UNIQUE,
    displayname     VARCHAR(50) NOT NULL,
    bio             TEXT,
    hash            BYTEA NOT NULL,
    salt            BYTEA NOT NULL,
    flags           INTEGER DEFAULT 0,
    created_at      TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    pfp             TEXT,
    FOREIGN KEY (pfp) REFERENCES UserFiles(hash)
);

CREATE TABLE IF NOT EXISTS Groups(
    group_id        SERIAL PRIMARY KEY,
    owner_id        INTEGER,
    name            VARCHAR(50) NOT NULL,
    "desc"          TEXT,
    flags           INTEGER DEFAULT 0,
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
    msg_id          SERIAL PRIMARY KEY,
    user_id         INTEGER,
    group_id        INTEGER,
    content         TEXT,
    timestamp       TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    attachments     JSON DEFAULT NULL,
    flags           INTEGER DEFAULT 0,
    parent_msg_id   INTEGER DEFAULT NULL,
    FOREIGN KEY (user_id) REFERENCES Users(user_id),
    FOREIGN KEY (group_id) REFERENCES Groups(group_id),
    FOREIGN KEY (user_id, group_id) REFERENCES GroupMembers(user_id, group_id),
    FOREIGN KEY (parent_msg_id) REFERENCES Messages(msg_id),
    CHECK (parent_msg_id != msg_id)
);

CREATE TABLE IF NOT EXISTS GroupCodes(
    invite_code VARCHAR(8) PRIMARY KEY DEFAULT ENCODE(gen_random_bytes(4), 'hex'),
    group_id    INT NOT NULL,
    uses        INT DEFAULT -1,
    FOREIGN KEY (group_id) REFERENCES Groups(group_id)
);

CREATE OR REPLACE FUNCTION insert_owner_group_member()
RETURNS TRIGGER AS $$
BEGIN 
    INSERT INTO GroupMembers(user_id, group_id)
    VALUES (NEW.owner_id, NEW.group_id);
    RETURN NULL;
END;
$$ LANGUAGE plpgsql;

CREATE OR REPLACE TRIGGER insert_owner_group_member
AFTER INSERT ON Groups 
FOR EACH ROW
EXECUTE FUNCTION insert_owner_group_member();

CREATE OR REPLACE FUNCTION delete_userfiles_if_ref_is_zero()
RETURNS TRIGGER AS $$
BEGIN
    DELETE FROM UserFiles
    WHERE ref_count <= 0;
    RETURN NULL;
END;
$$ LANGUAGE plpgsql;

CREATE OR REPLACE TRIGGER delete_userfiles_if_ref_is_zero
AFTER UPDATE OF ref_count ON UserFiles
FOR EACH ROW
EXECUTE FUNCTION delete_userfiles_if_ref_is_zero();
