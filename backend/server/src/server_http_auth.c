#include "server_http_auth.h"
#include "server_http.h"
#include "server.h"
#include "chat/db_user_session.h"
#include "chat/db_user.h"

#define ERR_MSG_INCORRECT "Incorrect username or password"

/**
 *  HTTP Respond OK or Unauthorized if it found the session or not from database.
 */
static const char*
after_get_session(UNUSED eworker_t* ew, dbcmd_ctx_t* ctx)
{
    client_t* client = ctx->param.client;

    if (ctx->ret == DB_ASYNC_ERROR)
    {
        server_http_resp(client, HTTP_CODE_UNAUTHORIZED);
        return NULL;
    }

    server_http_resp(client, HTTP_CODE_OK);

    return NULL;
}

/**
 *  Async select session id from database.
 *  Then calls `after_get_session()`
 */
static inline enum client_recv_status
server_handle_auth_session(eworker_t* ew, client_t* client, http_t* http)
{
    dbsession_t* session;
    dbcmd_ctx_t ctx = {
        .exec = after_get_session,
        .param.client = client,
    };

    if (http->session_uuid == NULL)
    {
        server_http_resp(client, HTTP_CODE_UNAUTHORIZED);
        return RECV_DISCONNECT;
    }

    session = calloc(1, sizeof(dbsession_t));
    strncpy(session->uuid, http->session_uuid, UUID_LEN - 1);

    if (!db_async_select_session(&ew->db, session, &ctx))
    {
        server_http_resp(client, HTTP_CODE_INTERAL_ERROR);
        return RECV_ERROR;
    }

    return RECV_OK;
}

/**
 *  After inserting session for user.
 *  HTTP respond with `Set-Cookie`
 */
static const char* 
after_session_insert(UNUSED eworker_t* ew, dbcmd_ctx_t* ctx)
{
    client_t* client = ctx->client;
    http_t* http;
    const char* session_uuid = ctx->param.session_id;
    char* set_cookie;

    if (ctx->ret == DB_ASYNC_ERROR)
    {
        server_http_resp(client, HTTP_CODE_INTERAL_ERROR);
        return NULL;
    }

    http = http_new_resp(HTTP_CODE_OK, NULL, 0);
    set_cookie = http_add_header(http, "Set-Cookie", NULL);
    snprintf(set_cookie, HTTP_HEAD_VAL_LEN - 1, "session_id=%s; HttpOnly; Secure; SameSite=Strict", session_uuid);

    http_send(client, http);

    http_free(http);

    return NULL;
}

/**
 *  Async insert session for user.
 *  Then calls `after_session_insert()`
 */
static inline const char*
async_create_session(eworker_t* ew, client_t* client, dbuser_t* user)
{
    dbcmd_ctx_t ctx = {
        .exec = after_session_insert,
        .client = client
    };
    if (!db_async_insert_session(&ew->db, user->user_id, &ctx))
        server_http_resp(client, HTTP_CODE_INTERAL_ERROR);

    return NULL;
}

/**
 *  After SELECT user from Users.
 *  SHA512 given password.
 *  Compare the given password SHA512 with database SHA512.
 *  If match, call `async_create_session()`
 */
static const char*
do_client_login(eworker_t* ew, dbcmd_ctx_t* ctx)
{
    dbuser_t* user = ctx->data;
    const char* password = ctx->param.user_login.password;
    client_t* client = ctx->client;
    u8 hash_login[SERVER_HASH_SIZE];

    if (ctx->ret == DB_ASYNC_ERROR)
    {
        server_http_resp_error(client, HTTP_CODE_UNAUTHORIZED, ERR_MSG_INCORRECT);
        return NULL;
    }

    server_sha512(password, user->salt, hash_login);

    if (memcmp(user->hash, hash_login, SERVER_HASH_SIZE) == 0)
        async_create_session(ew, client, user);
    else
        server_http_resp_error(client, HTTP_CODE_UNAUTHORIZED, ERR_MSG_INCORRECT);

    return NULL;
}

/**
 *  Async get user using username from database.
 *  Then calls `do_client_login()`
 */
static inline enum client_recv_status
server_handle_auth_login(eworker_t* ew, 
                         client_t* client, 
                         const char* username, 
                         const char* password, 
                         bool remember_me)
{
    dbcmd_ctx_t ctx = {
        .exec = do_client_login,
        .param.user_login.remember_me = remember_me
    };
    strncpy(ctx.param.user_login.password, password, DB_PASSWORD_MAX - 1);

    if (!db_async_get_user_username(&ew->db, username, &ctx))
    {
        server_http_resp(client, HTTP_CODE_INTERAL_ERROR);
        return RECV_ERROR;
    }

    return RECV_OK;
}

// static inline enum client_recv_status
// server_handle_auth_register(eworker_t* ew, client_t* client, http_t* http)
// {
//     return RECV_OK;
// }

static inline enum client_recv_status
handle_post(eworker_t* ew, client_t* client, http_t* http)
{
    json_object* payload;
    json_object* json_username;
    json_object* json_password;
    json_object* json_remember_me;
    const char* username;
    const char* password;
    bool  remember_me;
    json_tokener* tok;
    enum client_recv_status ret;

    if (http->body == NULL)
    {
        server_http_resp(client, HTTP_CODE_BAD_REQ);
        return RECV_DISCONNECT;
    }

    tok = json_tokener_new();
    payload = json_tokener_parse_ex(tok, http->body, http->body_len);
    json_tokener_free(tok);

    if (payload == NULL)
    {
        server_http_resp(client, HTTP_CODE_BAD_REQ);
        return RECV_DISCONNECT;
    }

    /**
     * Check for "username" key and type string 
     **/
    json_username = json_object_object_get(payload, "username");
    if (json_object_is_type(json_username, json_type_string) == 0)
        goto bad_req;
    username = json_object_get_string(json_username);

    /**
     * Check for "password" key and type string 
     **/
    json_password = json_object_object_get(payload, "password");
    if (json_object_is_type(json_password, json_type_string) == 0)
        goto bad_req;
    password = json_object_get_string(json_password);

    /**
     * Check for "remember_me" key and type bool 
     **/
    json_remember_me = json_object_object_get(payload, "remember_me");
    if (json_object_is_type(json_object_object_get(payload, "remember_me"), json_type_boolean) == 0)
        goto bad_req;
    remember_me = json_object_get_boolean(json_remember_me);

    if (strcmp(http->req.url, "/api/auth/login") == 0)
    {
        ret = server_handle_auth_login(ew, client, username, password, remember_me);
        goto free_payload;
    }

    // if (strcmp(http->req.url, "/api/auth/register"))
    //     return server_handle_auth_register(ew, client, http);

    return RECV_DISCONNECT;
bad_req:
    server_http_resp(client, HTTP_CODE_BAD_REQ);
    ret = RECV_DISCONNECT;
free_payload:
    json_object_put(payload);
    return ret;
}

enum client_recv_status 
server_handle_auth(eworker_t* ew, client_t* client, http_t* http)
{
    if (strcmp(http->req.method, "GET") == 0)
    {
        if (strcmp(http->req.url, "/api/auth/session") == 0)
            return server_handle_auth_session(ew, client, http);
    }
    else if (strcmp(http->req.method, "POST") == 0)
    {
        return handle_post(ew, client, http);
    }
    else 
    {
        server_http_resp(client, HTTP_CODE_METH_NOT_ALLOW);
        return RECV_DISCONNECT;
    }

    server_http_resp(client, HTTP_CODE_NOT_FOUND);
    return RECV_DISCONNECT;
}
