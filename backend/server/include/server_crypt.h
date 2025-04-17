#ifndef _SERVER_CRYPT_H_
#define _SERVER_CRYPT_H_

#include "common.h"
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#define SERVER_SALT_SIZE        (16)
#define SERVER_HASH_SIZE (EVP_MAX_MD_SIZE)
#define SERVER_HASH256_STR_SIZE (SHA256_DIGEST_LENGTH * 2 + 1)

void server_sha512(const char* secret, u8* salt, u8* hash);
void server_sha256(const void* data, size_t size, u8 hash_out[SHA256_DIGEST_LENGTH]);
char* server_compute_websocket_key(const char* websocket_key);
i32 server_secure_random(void* buf, u32 size);

#endif // _SERVER_CRYPT_H_
