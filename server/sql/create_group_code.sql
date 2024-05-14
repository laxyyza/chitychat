-- Params:
    -- $1::int = group_id
    -- $2::int = max_uses (-1 for infinite uses)
    -- $3::int = user_id (the creator)
-- Create Group Code only if user ($3) is a member.

INSERT INTO GroupCodes(group_id, max_uses)
SELECT $1::int, $2::int
FROM GroupMembers gm
WHERE gm.group_id = $1::int AND gm.user_id = $3::int
RETURNING invite_code;
