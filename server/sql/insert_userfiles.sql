INSERT INTO UserFiles(hash, name, size, mime_type)
VALUES (?, ?, ?, ?)
ON CONFLICT(hash) DO UPDATE SET 
    ref_count = ref_count + 1;