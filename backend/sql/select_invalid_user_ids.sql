SELECT input_id
FROM unnest($1::int[]) AS input_id
LEFT JOIN Users u ON u.user_id = input_id
WHERE u.user_id IS NULL;
