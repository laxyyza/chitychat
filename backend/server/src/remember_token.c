#include "remember_token.h"
#include "server_util.h"
#include "server_crypt.h"
#include "server.h"
#include "chat/db_remember_token.h"

remember_token_t*
create_remember_token(u32 user_id)
{
    remember_token_t* rt;
    u8 token[TOKEN_LEN];

    if (server_secure_random(token, TOKEN_LEN) == -1)
        return NULL;

    rt = malloc(sizeof(remember_token_t));
    rt->user_id = user_id;

    bytes_to_hex(token, TOKEN_LEN, rt->token_hex);

    server_sha256(token, TOKEN_LEN, rt->token_hash);

    return rt;
}

static const char*
after_token_insert(eworker_t* ew, dbcmd_ctx_t* ctx)
{
    client_t* client = ctx->param.remember_token.client;
    rt_callback_t callback = ctx->param.remember_token.callback;

    if (ctx->ret == DB_ASYNC_ERROR)
    {
        callback(ew, client, NULL, NULL);
        return NULL;
    }

    // TODO: create session id.
    callback(ew, client, ctx->data, NULL);

    return NULL;
}

bool 
server_rt_create_insert_token(eworker_t* ew, u32 user_id, client_t* client, rt_callback_t callback)
{
    remember_token_t* rt;

    if ((rt = create_remember_token(user_id)) == NULL)
        return false;

    dbcmd_ctx_t ctx = {
        .exec = after_token_insert,
        .param.remember_token.client = client,
        .param.remember_token.callback = callback
    };
    if (!db_async_insert_remember_token(&ew->db, rt, &ctx))
    {
        free(rt);
        return false;
    }

    return true;
}

// bool 
// server_rt_get_rotate_token(eworker_t* ew, client_t* client, const char* token, rt_callback_t callback)
// {
//
// }

// void 
// insert_remember_token(eworker_t* ew, client_t* client, u32 user_id)
// {
//     insert_remember_token(ew, user_id, callback)
// }


/*

3 operations:

1. insert(user_id) -> new token.
2. get_update(token) -> updated token.
3. delete(token) -> delete token.


insert:

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
