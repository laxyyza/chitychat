#include "server_crypt.h"
#include "server_http.h"

void 
server_hash(const char* secret, u8* salt, u8* hash)
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