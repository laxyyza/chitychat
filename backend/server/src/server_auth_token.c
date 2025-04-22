#include "server_auth_token.h"
#include "server_util.h"
#include "server_crypt.h"
#include "server.h"
#include "db/db_remember_token.h"
#include "db/db_remember_token_usage.h"

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
                        u32 user_id, 
                        remember_token_t* rt, 
                        auth_callback_t callback)
{
    char session_uuid[UUID_LEN];
    server_uuid_v4(session_uuid);

    server_redis_set_session(&ew->redis, session_uuid, user_id);

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

static const char*
after_token_update(eworker_t* ew, dbcmd_ctx_t* ctx)
{
    client_t* client = ctx->param.remember_token.client;
    u32 user_id = ctx->param.remember_token.user_id;
    remember_token_t* rt = ctx->data;
    auth_callback_t callback = ctx->param.remember_token.callback;

    if (ctx->ret == DB_ASYNC_ERROR)
    {
        server_http_resp(client, HTTP_CODE_INTERAL_ERROR);
        return NULL;
    }

    callback_create_session(ew, client, user_id, rt, callback);

    return NULL;
}

static inline void 
async_rotate_token(eworker_t* ew, u32 token_id, remember_token_param_t* param)
{
    client_t* client = param->client;
    dbcmd_ctx_t ctx = {
        .exec = after_token_update,
        .param.remember_token = *param
    };
    remember_token_t* rt;

    rt = create_remember_token(param->user_id);
    rt->token_id = token_id;

    if (!db_async_update_remember_token(&ew->db, rt, &ctx))
        server_http_resp(client, HTTP_CODE_INTERAL_ERROR);

    db_async_remember_token_usage(&ew->db, token_id, client->user_agent, client->addr.ip_str);
}

static const char*
after_token_select(eworker_t* ew, dbcmd_ctx_t* ctx)
{
    client_t* client = ctx->param.remember_token.client;
    u32 token_id = ctx->param.remember_token.token_id;

    if (ctx->ret == DB_ASYNC_ERROR)
    {
        server_http_resp(client, HTTP_CODE_UNAUTHORIZED);
        return NULL;
    }

    async_rotate_token(ew, token_id, &ctx->param.remember_token);

    return NULL;
}

bool 
server_auth_token_get_rotate(eworker_t* ew, client_t* client, const u8* token, auth_callback_t callback)
{
    dbcmd_ctx_t ctx = {
        .exec = after_token_select,
        .param.remember_token.client = client,
        .param.remember_token.callback = callback,
    };
    if (!db_async_select_remember_token(&ew->db, token, &ctx))
        return false;

    return true;
}
