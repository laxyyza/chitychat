#include "server_auth_token.h"
#include "server_util.h"
#include "server_crypt.h"
#include "server.h"
#include "chat/db_remember_token.h"

remember_token_t*
create_remember_token(u32 user_id)
{
    remember_token_t* auth_token;
    u8 token[TOKEN_LEN];

    if (server_secure_random(token, TOKEN_LEN) == -1)
        return NULL;

    auth_token = malloc(sizeof(remember_token_t));
    auth_token->user_id = user_id;

    bytes_to_hex(token, TOKEN_LEN, auth_token->token_hex);

    server_sha256(token, TOKEN_LEN, auth_token->token_hash);

    return auth_token;
}

static inline void 
callback_create_session(eworker_t* ew, 
                        client_t* client, 
                        UNUSED u32 user_id, 
                        remember_token_t* rt, 
                        auth_callback_t callback)
{
    char session_uuid[UUID_LEN];
    server_uuid_v4(session_uuid);

    // TODO: Set session id in Redis.
    callback(ew, client, rt, session_uuid);
}

static const char*
after_token_inseauth_token(eworker_t* ew, dbcmd_ctx_t* ctx)
{
    client_t* client = ctx->param.remember_token.client;
    auth_callback_t callback = ctx->param.remember_token.callback;
    u32 user_id = ctx->param.remember_token.user_id;

    if (ctx->ret == DB_ASYNC_ERROR)
    {
        callback(ew, client, NULL, NULL);
        return NULL;
    }

    callback_create_session(ew, client, user_id, ctx->data, callback);

    return NULL;
}

static inline bool
auth_create_remember_token(eworker_t* ew, u32 user_id, client_t* client, auth_callback_t callback)
{
    remember_token_t* rt;

    if ((rt = create_remember_token(user_id)) == NULL)
        return false;

    dbcmd_ctx_t ctx = {
        .exec = after_token_inseauth_token,
        .param.remember_token.client = client,
        .param.remember_token.callback = callback,
        .param.remember_token.user_id = user_id
    };
    if (!db_async_insert_remember_token(&ew->db, rt, &ctx))
    {
        free(rt);
        return false;
    }

    return true;
}

bool 
server_auth_token_create(eworker_t* ew, u32 user_id, client_t* client, bool remember_me, auth_callback_t callback)
{
    if (remember_me)
        return auth_create_remember_token(ew, user_id, client, callback);

    callback_create_session(ew, client, user_id, NULL, callback);
    return true;
}

// bool 
// server_auth_token_get_rotate_token(eworker_t* ew, client_t* client, const char* token, auth_callback_t callback)
// {
//
// }

// void 
// inseauth_token_remember_token(eworker_t* ew, client_t* client, u32 user_id)
// {
//     inseauth_token_remember_token(ew, user_id, callback)
// }


/*

3 operations:

1. inseauth_token(user_id) -> new token.
2. get_update(token) -> updated token.
3. delete(token) -> delete token.


inseauth_token:

callback(client, remember_token, session)
{
    if (token == NULL && session == NULL)
        return resp_internal_error

    new http
    if (session)
        http.set_cookie(session_id=session.id) // only until browser close
    if (remember_token)
        http.set_cookie(remember_token=remember_token) // 30 days
    send(client, http)
}

// creates new token and session id.
// create new token in postgres.
// create new session id in redis.
create_remember_token(user_id, client, callback)

// creates new session id and rotates token.
// select from postgres
// create new session in redis.
get_update(client, token, callback)

// delete token.
delete(token)

*/
