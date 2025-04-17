#include "server_crypt.h"
#include "server_http.h"

#define UUID_BUF_LEN 16

void 
server_sha512(const char* secret, u8* salt, u8* hash)
{
    EVP_MD_CTX* mdctx; 
    const EVP_MD* md = EVP_sha512();
    
    mdctx = EVP_MD_CTX_new();

    EVP_DigestInit_ex(mdctx, md, NULL);
    if (salt)
        EVP_DigestUpdate(mdctx, salt, SERVER_SALT_SIZE);
    EVP_DigestUpdate(mdctx, secret, strlen(secret));
    EVP_DigestFinal_ex(mdctx, hash, NULL);
    EVP_MD_CTX_free(mdctx);
}

void
server_sha256(const void* data, size_t size, u8 hash_out[SHA256_DIGEST_LENGTH])
{
    SHA256_CTX sha256;
    
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, data, size);
    SHA256_Final(hash_out, &sha256);
}

char* 
server_compute_websocket_key(const char* websocket_key)
{
    char* concatenated;
    BIO* bio; 
    BIO* b64;
    BUF_MEM* bufferPtr;
    char *b64_data;
    unsigned char sha1_hash[SHA_DIGEST_LENGTH];

    concatenated = malloc(strlen(websocket_key) + sizeof(WEBSOCKET_GUID) + 1);
    strcpy(concatenated, websocket_key);
    strcat(concatenated, WEBSOCKET_GUID);

    SHA1((u8*)concatenated, strlen(concatenated), sha1_hash);
    free(concatenated);

    bio = BIO_new(BIO_s_mem());
    b64 = BIO_new(BIO_f_base64());
    bio = BIO_push(b64, bio);

    BIO_write(bio, sha1_hash, sizeof(sha1_hash));
    BIO_flush(bio);
    BIO_get_mem_ptr(bio, &bufferPtr);

    b64_data = malloc(bufferPtr->length);
    memcpy(b64_data, bufferPtr->data, bufferPtr->length);
    b64_data[bufferPtr->length - 1] = 0x00;

    BIO_free_all(bio);

    return b64_data;
}

i32
server_secure_random(void* buf, u32 size)
{
    i32 ret;
    if ((ret = getrandom(buf, size, 0)) == -1)
        error("getrandom: %s\n", ERRSTR);
    return ret;
}

void 
server_uuid_v4(char uuid_out[UUID_LEN])
{
    u8 uuid_bytes[UUID_BUF_LEN];

    server_secure_random(uuid_bytes, UUID_BUF_LEN);

    // Set version to 4 (random)
    uuid_bytes[6] = (uuid_bytes[6] & 0x0F) | 0x40;
    // Set variant to 10xx
    uuid_bytes[8] = (uuid_bytes[8] & 0x3F) | 0x80;

    snprintf(uuid_out, UUID_LEN,
        "%02x%02x%02x%02x-"
        "%02x%02x-"
        "%02x%02x-"
        "%02x%02x-"
        "%02x%02x%02x%02x%02x%02x",
        uuid_bytes[0], uuid_bytes[1], 
        uuid_bytes[2], uuid_bytes[3],
        uuid_bytes[4], uuid_bytes[5],
        uuid_bytes[6], uuid_bytes[7],
        uuid_bytes[8], uuid_bytes[9],
        uuid_bytes[10], uuid_bytes[11], 
        uuid_bytes[12], uuid_bytes[13], 
        uuid_bytes[14], uuid_bytes[15]);
}
