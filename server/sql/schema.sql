SET CONSTRAINTS ALL DEFERRED;

CREATE EXTENSION IF NOT EXISTS pgcrypto;

CREATE TABLE IF NOT EXISTS UserFiles(
    hash            text PRIMARY KEY UNIQUE,
    size            bigint NOT null,
    mime_type       text NOT null,
    ref_count       int DEFAULT 1
);

CREATE TABLE IF NOT EXISTS Users(
    user_id         SERIAL PRIMARY KEY,
    username        varchar(50) NOT null UNIQUE,
    displayname     varchar(50) NOT null,
    bio             text,
    hash            bytea NOT null,
    salt            bytea NOT null, 
    created_at      timestamp DEFAULT CURRENT_TIMESTAMP,
    pfp             text,
    FOREIGN KEY (pfp) REFERENCES UserFiles(hash)
);

CREATE TABLE IF NOT EXISTS Groups(
    group_id        SERIAL PRIMARY KEY,
    owner_id        int,
    name            varchar(50) NOT null,
    "desc"          text,
    public          boolean DEFAULT false,
    created_at      timestamp DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (owner_id) REFERENCES Users(user_id)
);

CREATE TABLE IF NOT EXISTS GroupMembers(
    user_id         int,
    group_id        int,
    join_date       timestamp DEFAULT CURRENT_TIMESTAMP,
    flags           int DEFAULT 0,
    PRIMARY KEY (user_id, group_id),
    FOREIGN KEY (user_id) REFERENCES Users(user_id),
    FOREIGN KEY (group_id) REFERENCES Groups(group_id)
);

CREATE TABLE IF NOT EXISTS Messages(
    msg_id          SERIAL PRIMARY KEY,
    user_id         int,
    group_id        int,
    content         text,
    timestamp       timestamp DEFAULT CURRENT_TIMESTAMP,
    attachments     json DEFAULT null,
    parent_msg_id   int DEFAULT null,
    FOREIGN KEY (user_id) REFERENCES Users(user_id),
    FOREIGN KEY (group_id) REFERENCES Groups(group_id),
    FOREIGN KEY (user_id, group_id) REFERENCES GroupMembers(user_id, group_id),
    FOREIGN KEY (parent_msg_id) REFERENCES Messages(msg_id),
    CHECK (parent_msg_id != msg_id)
);

CREATE TABLE IF NOT EXISTS GroupCodes(
    invite_code VARCHAR(8) PRIMARY KEY DEFAULT ENCODE(gen_random_bytes(4), 'hex'),
    group_id    int NOT null,
    uses        int DEFAULT 0,
    max_uses    int NOT null,
    FOREIGN KEY (group_id) REFERENCES Groups(group_id)
);

CREATE OR REPLACE FUNCTION delete_groupcode_if_over_max()
RETURNS TRIGGER AS $$
BEGIN
    DELETE FROM GroupCodes
    WHERE max_uses != -1 AND uses >= max_uses;
    RETURN null;
END;
$$ LANGUAGE plpgsql;

CREATE OR REPLACE TRIGGER delete_groupcode_if_over_max
AFTER UPDATE OF uses ON GroupCodes
FOR EACH ROW
EXECUTE FUNCTION delete_groupcode_if_over_max();

CREATE OR REPLACE FUNCTION insert_owner_group_member()
RETURNS TRIGGER AS $$
BEGIN 
    INSERT INTO GroupMembers(user_id, group_id)
    VALUES (NEW.owner_id, NEW.group_id);
    RETURN null;
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
    RETURN null;
END;
$$ LANGUAGE plpgsql;

CREATE OR REPLACE TRIGGER delete_userfiles_if_ref_is_zero
AFTER UPDATE OF ref_count ON UserFiles
FOR EACH ROW
EXECUTE FUNCTION delete_userfiles_if_ref_is_zero();
