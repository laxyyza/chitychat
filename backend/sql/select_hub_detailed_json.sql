SELECT row_to_json(hub_data) AS hub
FROM (
    SELECT h.*,

    (
        SELECT json_agg(m.user_id)
        FROM HubMembers m
        WHERE m.hub_id = h.hub_id
    ) AS member_ids,

    (
        SELECT json_agg(
            jsonb_build_object(
                'category_id', c.category_id,
                'name', c.name,
                'position', c.position,
                'channels', (
                    SELECT json_agg(ch)
                    FROM HubChannels ch
                    WHERE ch.category_id = c.category_id
                )
            )
        )
        FROM HubCategories c
        WHERE c.hub_id = h.hub_id
    ) AS categories

    FROM Hubs h 
    JOIN HubMembers hm ON hm.hub_id = h.hub_id
    WHERE h.hub_id = $1::int AND hm.user_id = $2::int
) AS hub_data;
