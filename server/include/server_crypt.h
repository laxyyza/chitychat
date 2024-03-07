#ifndef _SERVER_CRYPT_H_
#define _SERVER_CRYPT_H_

#include "common.h"
#include <openssl/evp.h>
#include <sys/random.h>

#define SERVER_SALT_SIZE 16
#define SERVER_HASH_SIZE EVP_MAX_MD_SIZE

void server_hash(const char* secret, u8* salt, u8* hash);
char* server_compute_websocket_key(const char* websocket_key);

#endif // _SERVER_CRYPT_H_