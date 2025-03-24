-- Params
    -- $1::int = group_id
    -- $2::int = user_id (the user who requested this)
-- Get all the groups codes in group if only user ($2) is a member.

SELECT json_agg(gc_row)
FROM (
    SELECT invite_code AS code, uses, max_uses
    FROM GroupCodes gc
    INNER JOIN GroupMembers gm ON gc.group_id = gm.group_id
    WHERE gc.group_id = $1::int AND gm.user_id = $2::int
) gc_row;
