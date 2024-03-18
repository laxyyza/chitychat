UPDATE Users
SET 
username =      CASE WHEN ? THEN ? ELSE username END,
displayname =   CASE WHEN ? THEN ? ELSE displayname END,
pfp =           CASE WHEN ? THEN ? ELSE pfp END
WHERE user_id = ?