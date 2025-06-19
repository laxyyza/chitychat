SELECT json_agg(h.*) FROM Hubs h 
JOIN HubMembers hm ON hm.hub_id = h.hub_id 
WHERE hm.user_id = $1::int;
