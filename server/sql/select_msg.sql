-- SELECT * FROM Messages WHERE msg_id = ?

SELECT DISTINCT m.*
FROM Messages m
JOIN Groups g ON m.group_id = g.group_id
WHERE m.group_id = ? OR m.msg_id = ?;