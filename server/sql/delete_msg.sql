-- Params
    -- $1::int  = msg_id
    -- $2::int  = user_id (the user who requested this)
-- Delete message if only msg.user_id = $2 OR user is group owner.
-- Return group_id 

DELETE FROM Messages m
USING Groups g
WHERE m.msg_id = $1::int
    AND m.group_id = g.group_id
    AND (m.user_id = $2::int OR g.owner_id = $2::int)
RETURNING g.group_id, m.attachments;
