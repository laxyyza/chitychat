SELECT user_id FROM Sessions 
WHERE session_id = $1::UUID AND expires_at > NOW();