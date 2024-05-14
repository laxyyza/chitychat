-- Params
    -- $1::varchar(8) = invite_code
    -- $2::int        = user_id (the user who tries to delete)

DELETE FROM GroupCodes gc
USING GroupMembers gm
WHERE gc.invite_code = $1::varchar(8)
    AND gc.group_id = gm.group_id
    AND gm.user_id = $2::int;
