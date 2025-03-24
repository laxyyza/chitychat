#include "server_crypt.h"
#include "server_http.h"

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
server_sha256_str(const void* data, size_t size, char* output)
{
    u8 hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, data, size);
    SHA256_Final(hash, &sha256);

    for (size_t i = 0; i < SHA256_DIGEST_LENGTH; i++)
        sprintf(output + (i * 2), "%02x", hash[i]);
    output[SERVER_HASH256_STR_SIZE - 1] = 0x00;
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