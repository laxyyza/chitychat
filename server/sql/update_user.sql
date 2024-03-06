UPDATE Users
SET 
username =      CASE WHEN ? THEN ? ELSE username END,
displayname =   CASE WHEN ? THEN ? ELSE displayname END,
pfp_name =      CASE WHEN ? THEN ? ELSE pfp_name END
WHERE user_id = ?