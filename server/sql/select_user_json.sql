-- Select user(s) as json array

SELECT json_agg(row_to_json(user_row))
FROM (
    SELECT user_id, username, displayname, bio, pfp AS pfp_name
    FROM Users
    WHERE user_id in (
        SELECT json_array_elements_text($1::json)::int
    )
) user_row;
