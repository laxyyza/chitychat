#ifndef _SERVER_AUTH_TOKEN_H_
#define _SERVER_AUTH_TOKEN_H_

#include "common.h"
#include "server_crypt.h"
#include "server_client.h"

#define TOKEN_LEN SHA256_DIGEST_LENGTH
#define TOKEN_HEX_LEN (TOKEN_LEN * 2)
#define TOKEN_EXPIRE_TIME_DAYS 30   // TODO: Make it config settable.
#define TOKEN_EXPIRE_STR_LEN 30

typedef struct 
{
    /* The original 256-bit token, encoded as hex.
     * This is sent to the user and stored in their cookie. */
    char token_hex[TOKEN_HEX_LEN + 1];

    /* SHA-256 hash of the original token.
     * This is stored in the database for verification. */
    u8 token_hash[TOKEN_LEN];

    u32 user_id;
    u32 token_id;

    char expires[TOKEN_EXPIRE_STR_LEN];
} remember_token_t;

typedef void (*auth_callback_t)(eworker_t* ew, client_t* client, remember_token_t* rt, const char* session);

remember_token_t* create_remember_token(u32 user_id);
bool server_auth_token_create(eworker_t* ew, u32 user_id, client_t* client, bool remember_me, auth_callback_t callback);
bool server_auth_token_get_rotate(eworker_t* ew, client_t* client, const u8* token, auth_callback_t callback);
bool server_auth_token_delete(eworker_t* ew, const char* token);

#endif // _SERVER_AUTH_TOKEN_H_
