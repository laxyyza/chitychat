SELECT
    h.name AS hub_name,
    (
        SELECT COUNT (*) FROM HubMembers hm WHERE hm.hub_id = h.hub_id 
    ) AS members_count,
    i.expires_at,
    u.username,
    u.displayname
FROM HubInvites i 
JOIN Hubs h ON h.hub_id = i.hub_id 
JOIN Users u ON u.user_id = i.created_by
WHERE i.code = $1::text;
