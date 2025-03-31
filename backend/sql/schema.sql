SET CONSTRAINTS ALL DEFERRED;

CREATE EXTENSION IF NOT EXISTS pgcrypto;

CREATE TABLE IF NOT EXISTS Attachments(
    attachment_id   SERIAL PRIMARY KEY,
    user_id         int NOT NULL,
    file_name       text NOT NULL,
    file_size       bigint NOT NULL,
    mime_type       text NOT NULL,
    storage_path    text NOT NULL,
    created_at      TIMESTAMP DEFAULT NOW()

    -- CONSTRAINT fk_user_id FOREIGN KEY (user_id) REFERENCES Users(user_id) DEFERRABLE INITIALLY DEFERRED
);

CREATE TABLE IF NOT EXISTS Users(
    user_id         SERIAL PRIMARY KEY,
    username        varchar(50) NOT null UNIQUE,
    displayname     varchar(50) NOT null,
    bio             text,
    hash            bytea NOT null,
    salt            bytea NOT null, 
    created_at      timestamp DEFAULT CURRENT_TIMESTAMP,
    pfp             int,
    FOREIGN KEY (pfp) REFERENCES Attachments(attachment_id)
);

DO $$ BEGIN
    ALTER TABLE Attachments ADD CONSTRAINT user_id_fkey FOREIGN KEY (user_id) REFERENCES Users(user_id);
EXCEPTION
    WHEN duplicate_object THEN null;
END $$;

CREATE TABLE IF NOT EXISTS Groups(
    group_id        SERIAL PRIMARY KEY,
    owner_id        int,
    name            varchar(50) NOT null,
    "desc"          text,
    created_at      timestamp DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (owner_id) REFERENCES Users(user_id) ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS GroupMembers(
    user_id         int,
    group_id        int,
    join_date       timestamp DEFAULT CURRENT_TIMESTAMP,
    flags           int DEFAULT 0,
    PRIMARY KEY (user_id, group_id),
    FOREIGN KEY (user_id) REFERENCES Users(user_id) ON DELETE CASCADE,
    FOREIGN KEY (group_id) REFERENCES Groups(group_id) ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS Hubs(
    hub_id      SERIAL PRIMARY KEY,
    owner_id    int NOT NULL,
    name        text NOT NULL,
    public      boolean DEFAULT false,
    settings    json DEFAULT '{}',
    created_at  TIMESTAMP DEFAULT NOW(),

    FOREIGN KEY (owner_id) REFERENCES Users(user_id) ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS HubMembers(
    user_id     int,
    hub_id      int,
    roles       json DEFAULT '{}',
    created_at  TIMESTAMP DEFAULT NOW(),

    PRIMARY KEY (user_id, hub_id),
    FOREIGN KEY (user_id) REFERENCES Users(user_id) ON DELETE CASCADE,
    FOREIGN KEY (hub_id)  REFERENCES Hubs(hub_id) ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS DirectMessages(
    user1_id    int NOT NULL,
    user2_id    int NOT NULL,
    created_at  TIMESTAMP DEFAULT NOW(),

    PRIMARY KEY (user1_id, user2_id),
	UNIQUE (user1_id, user2_id),
    FOREIGN KEY (user1_id) REFERENCES Users(user_id),
    FOREIGN KEY (user2_id) REFERENCES Users(user_id)
);

DO $$ BEGIN
    CREATE TYPE channel_type AS ENUM ('DM', 'GROUP', 'HUB');
EXCEPTION
    WHEN duplicate_object THEN null;
END $$;

CREATE TABLE IF NOT EXISTS TextChannels(
    channel_id      SERIAL PRIMARY KEY,
    type            channel_type NOT NULL
);

CREATE TABLE IF NOT EXISTS HubChannels(
    channel_id      SERIAL PRIMARY KEY,
    hub_id          int NOT NULL,
    name            TEXT NOT NULL,
    created_at      TIMESTAMP DEFAULT NOW(),
    settings        json DEFAULT '{}',

    FOREIGN KEY (channel_id) REFERENCES TextChannels(channel_id) ON DELETE CASCADE,
    FOREIGN KEY (hub_id) REFERENCES Hubs(hub_id) ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS Messages(
    msg_id          SERIAL PRIMARY KEY,
    user_id         int,
    channel_id      int,
    content         text,
    timestamp       timestamp DEFAULT CURRENT_TIMESTAMP,
    attachments     json DEFAULT null,
    parent_msg_id   int DEFAULT null,
    FOREIGN KEY (user_id) REFERENCES Users(user_id),
    FOREIGN KEY (channel_id) REFERENCES TextChannels(channel_id) ON DELETE CASCADE,
    FOREIGN KEY (parent_msg_id) REFERENCES Messages(msg_id),
    CHECK (parent_msg_id != msg_id)
);

CREATE TABLE IF NOT EXISTS GroupCodes(
    invite_code VARCHAR(8) PRIMARY KEY DEFAULT ENCODE(gen_random_bytes(4), 'hex'),
    group_id    int NOT null,
    uses        int DEFAULT 0,
    max_uses    int NOT null,
    FOREIGN KEY (group_id) REFERENCES Groups(group_id) ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS Sessions(
    session_id  UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    user_id     int NOT NULL REFERENCES Users(user_id) ON DELETE CASCADE,
    created_at  TIMESTAMP DEFAULT now(),
    last_used   TIMESTAMP DEFAULT now(),
    expires_at  TIMESTAMP DEFAULT now() + INTERVAL '7 days'
);

DO $$ BEGIN
    CREATE TYPE friendship_status AS ENUM ('PENDING', 'ACCEPTED', 'BLOCKED');
EXCEPTION
    WHEN duplicate_object THEN null;
END $$;

CREATE TABLE IF NOT EXISTS Friendships(
    friendship_id   SERIAL PRIMARY KEY,
    source_user_id  int NOT NULL REFERENCES Users(user_id) ON DELETE CASCADE,
    target_user_id  int NOT NULL REFERENCES Users(user_id) ON DELETE CASCADE,
    status          friendship_status NOT NULL DEFAULT 'PENDING',
    created_at      TIMESTAMP DEFAULT NOW(),

    UNIQUE (source_user_id, target_user_id)
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