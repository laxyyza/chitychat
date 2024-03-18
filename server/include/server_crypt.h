#ifndef _SERVER_CRYPT_H_
#define _SERVER_CRYPT_H_

#include "common.h"
#include <openssl/evp.h>
#include <sys/random.h>

#define SERVER_SALT_SIZE        (16)
#define SERVER_HASH_SIZE        (EVP_MAX_MD_SIZE)
#define SERVER_HASH256_STR_SIZE (SHA256_DIGEST_LENGTH * 2 + 1)

void server_sha512(const char* secret, u8* salt, u8* hash);
char* server_sha256_str(const void* data, size_t size);
char* server_compute_websocket_key(const char* websocket_key);

#endif // _SERVER_CRYPT_H_