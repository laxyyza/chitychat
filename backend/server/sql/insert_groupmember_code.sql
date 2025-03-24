-- Params
    -- $1::int = user_id
    -- $2::varchar(8) = invite_code
-- Return: group_id

-- Insert a group member via invite_code, increment group code uses, then return the group joined

-- Get GroupCode from invite_code ($2)
WITH gc AS (
    SELECT *
    FROM GroupCodes
    WHERE invite_code = $2::varchar(8)
),

-- Insert Group Member with user_id ($1), gc.group_id
insert_gm AS (
    INSERT INTO GroupMembers(user_id, group_id)
    VALUES(
        $1::int,
        (SELECT group_id FROM gc)
    )
),

-- Select everything from gc.group_id
g AS (
    SELECT *
    FROM Groups
    WHERE group_id = (SELECT group_id FROM gc)
)

-- Increment Group Code uses
UPDATE GroupCodes
SET uses = GroupCodes.uses + 1
FROM g, gc
WHERE GroupCodes.invite_code = gc.invite_code

-- Return group ID
RETURNING g.group_id;
