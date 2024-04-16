INSERT INTO UserFiles(hash, size, mime_type)
VALUES (
    $1::text, 
    $2::int, 
    $3::text
)
ON CONFLICT(hash) DO UPDATE 
SET ref_count = UserFiles.ref_count + 1;
