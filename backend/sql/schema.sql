SET CONSTRAINTS ALL DEFERRED;

CREATE EXTENSION IF NOT EXISTS pgcrypto;

CREATE TABLE IF NOT EXISTS Users(
    user_id         SERIAL PRIMARY KEY,
    username        varchar(50) NOT null UNIQUE,
    displayname     varchar(50) NOT null,
    bio             text,
    hash            bytea NOT null,
    salt            bytea NOT null, 
    created_at      timestamp DEFAULT CURRENT_TIMESTAMP,
    pfp_url         text
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

DO $$ BEGIN
    CREATE TYPE channel_type AS ENUM ('DM', 'GROUP', 'HUB');
EXCEPTION
    WHEN duplicate_object THEN null;
END $$;

CREATE TABLE IF NOT EXISTS TextChannels(
    channel_id      SERIAL PRIMARY KEY,
    type            channel_type NOT NULL,
    last_message    TIMESTAMP DEFAULT now()
);

CREATE TABLE IF NOT EXISTS Groups(
    group_id        SERIAL PRIMARY KEY,
    owner_id        int NOT NULL,
    channel_id      int NOT NULL,
    name            varchar(50) NOT null,
    "desc"          text,
    created_at      timestamp DEFAULT CURRENT_TIMESTAMP,

    FOREIGN KEY (owner_id) REFERENCES Users(user_id) ON DELETE CASCADE,
    FOREIGN KEY (channel_id) REFERENCES TextChannels(channel_id) ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS GroupMembers(
    user_id         int,
    group_id        int,
    join_date       timestamp DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (user_id, group_id),
    FOREIGN KEY (user_id) REFERENCES Users(user_id) ON DELETE CASCADE,
    FOREIGN KEY (group_id) REFERENCES Groups(group_id) ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS DirectMessages(
    user1_id    int NOT NULL,
    user2_id    int NOT NULL,
    channel_id  int NOT NULL,
    created_at  TIMESTAMP DEFAULT NOW(),

    PRIMARY KEY (user1_id, user2_id),
    FOREIGN KEY (channel_id) REFERENCES TextChannels(channel_id),
    FOREIGN KEY (user1_id) REFERENCES Users(user_id),
    FOREIGN KEY (user2_id) REFERENCES Users(user_id),

    CHECK (user1_id < user2_id)
);

CREATE UNIQUE INDEX IF NOT EXISTS direct_messages_unique_idx ON DirectMessages (
    LEAST(user1_id, user2_id),
    GREATEST(user1_id, user2_id)
);

CREATE TABLE IF NOT EXISTS HubCategories (
    category_id     SERIAL PRIMARY KEY,
    hub_id          int NOT NULL,
    name            TEXT NOT NULL,
    position        int NOT NULL DEFAULT 0,

    FOREIGN KEY (hub_id) REFERENCES Hubs(hub_id) ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS HubChannels(
    channel_id      int PRIMARY KEY NOT NULL DEFAULT NULL,
    hub_id          int NOT NULL,
    category_id     int NOT NULL,
    name            TEXT NOT NULL,
    created_at      TIMESTAMP DEFAULT NOW(),
    settings        json DEFAULT '{}',
    position        int NOT NULL DEFAULT 0,

    FOREIGN KEY (channel_id) REFERENCES TextChannels(channel_id) ON DELETE CASCADE,
    FOREIGN KEY (category_id) REFERENCES HubCategories(category_id) ON DELETE CASCADE,
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

CREATE TABLE IF NOT EXISTS RememberTokens(
    token_id    SERIAL PRIMARY KEY,
    user_id     int NOT NULL REFERENCES Users(user_id) ON DELETE CASCADE,
    token_hash  bytea NOT NULL,
    created_at  timestamp DEFAULT now(),
    expires_at  timestamp NOT NULL,
    last_used_at timestamp DEFAULT now()
);

CREATE INDEX IF NOT EXISTS idx_token_hash ON RememberTokens(token_hash);

CREATE TABLE IF NOT EXISTS RememberTokenUsage(
    usage_id    SERIAL PRIMARY KEY,
    token_id    int NOT NULL REFERENCES RememberTokens(token_id) ON DELETE CASCADE,
    user_agent  TEXT NOT NULL,
    ip_address  INET NOT NULL,
    timestamp   timestamp DEFAULT now()
);

CREATE TABLE IF NOT EXISTS LoginAttempts(
    login_id    SERIAL PRIMARY KEY,
    username    TEXT NOT NULL,
    successful  boolean NOT NULL,
    user_agent  TEXT NOT NULL,
    ip_address  INET NOT NULL,
    timestamp   timestamp DEFAULT now()
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

CREATE TABLE IF NOT EXISTS HubInvites(
    code        text        PRIMARY KEY,
    hub_id      int         NOT NULL REFERENCES Hubs(hub_id) ON DELETE CASCADE,
    created_by  int         NOT NULL REFERENCES Users(user_id) ON DELETE CASCADE,
    created_at  timestamp   DEFAULT now(),
    expires_at  timestamp,  -- nullable, means no expiration
    max_uses    int,        -- nullable, means unlimited uses
    use_count   int         NOT NULL DEFAULT 0
);

CREATE TABLE IF NOT EXISTS HubInviteUses(
    code        text NOT NULL REFERENCES HubInvites(code) ON DELETE CASCADE,
    user_id     int NOT NULL REFERENCES Users(user_id) ON DELETE CASCADE,
    joined_at   timestamp NOT NULL DEFAULT NOW(),
    PRIMARY KEY (code, user_id)
);

CREATE OR REPLACE FUNCTION increment_invite_use_count()
RETURNS TRIGGER AS $$
BEGIN 
    UPDATE HubInvites
    SET use_count = use_count + 1 
    WHERE code = NEW.code;
    RETURN NEW;
END;
$$ LANGUAGE plpgsql;

CREATE OR REPLACE TRIGGER trg_increment_use_count 
AFTER INSERT ON HubInviteUses
FOR EACH ROW
EXECUTE FUNCTION increment_invite_use_count();

-- Insert owner ID in GroupMembers after INSERT INTO Groups.
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

-- Insert owner ID in HubMembers after INSERT INTO Hubs.
CREATE OR REPLACE FUNCTION insert_owner_hub_member()
RETURNS TRIGGER AS $$
BEGIN 
    INSERT INTO HubMembers(user_id, hub_id)
    VALUES (NEW.owner_id, NEW.hub_id);
    RETURN null;
END;
$$ LANGUAGE plpgsql;

CREATE OR REPLACE TRIGGER insert_owner_hub_member 
AFTER INSERT ON Hubs 
FOR EACH ROW 
EXECUTE FUNCTION insert_owner_hub_member();

-- After INSERT INTO Messages, update TextChannels(last_message) with Messages(timestamp)
CREATE OR REPLACE FUNCTION update_textchannel_last_message()
RETURNS TRIGGER AS $$
BEGIN
    UPDATE TextChannels
    SET last_message = NEW.timestamp 
    WHERE channel_id = NEW.channel_id;
    RETURN NEW;
END;
$$ LANGUAGE plpgsql;

CREATE OR REPLACE TRIGGER update_last_message_at 
AFTER INSERT ON Messages 
FOR EACH ROW
EXECUTE FUNCTION update_textchannel_last_message();

-- INSERT INTO TextChannels before INSERT INTO HubChannels.
-- Then use that TextChannels(channel_id) in HubChannels(channel_id).
CREATE OR REPLACE FUNCTION insert_text_channel_and_link() 
RETURNS TRIGGER AS $$
DECLARE
    new_text_channel_id int;
BEGIN
    INSERT INTO TextChannels(type)
    VALUES ('HUB')
    RETURNING channel_id INTO new_text_channel_id;

    NEW.channel_id = new_text_channel_id;

    RETURN NEW;
END;
$$ LANGUAGE plpgsql;

CREATE OR REPLACE TRIGGER hub_channel_insert
BEFORE INSERT ON HubChannels 
FOR EACH ROW
WHEN (NEW.channel_id IS NULL)
EXECUTE FUNCTION insert_text_channel_and_link();
