#include "server_auth.h"
#include "server_http.h"
#include "server.h"
#include "db/db_user.h"
#include "db/db_login_attempts.h"
#include "server_auth_token.h"

#define ERR_MSG_INCORRECT "Incorrect username or password"

#define SET_COOKIE_REMEMBER_TOKEN "remember_token=%s; HttpOnly; Path=/api/auth/remember; Secure; SameSite=Strict; Expires=%s"
#define SET_COOKIE_SESSION "session=%s; HttpOnly; Path=/; Secure; SameSite=Strict"

static inline void 
auth_http_set_cookies(http_t* http, remember_token_t* rt, const char* session_uuid)
{
    char* set_cookie;

    if (rt)
    {
        set_cookie = http_add_header(http, "Set-Cookie", NULL);
        snprintf(set_cookie, HTTP_HEAD_VAL_LEN - 1, 
                 SET_COOKIE_REMEMBER_TOKEN,
                 rt->token_hex, rt->expires);
    }
    set_cookie = http_add_header_adv(http, "Set-Cookie", NULL, false);
    snprintf(set_cookie, HTTP_HEAD_VAL_LEN - 1, SET_COOKIE_SESSION, session_uuid);
}

static void 
after_token_create(UNUSED eworker_t* ew, client_t* client, remember_token_t* rt, const char* session_uuid)
{
    http_t* http;

    if (rt == NULL && session_uuid == NULL)
    {
        server_http_resp(client, HTTP_CODE_INTERAL_ERROR);
        return;
    }

    http = http_new_resp(HTTP_CODE_OK, NULL, 0);
    auth_http_set_cookies(http, rt, session_uuid);

    http_send(client, http);

    http_free(http);
}

static inline enum client_recv_status
server_handle_auth_remember(eworker_t* ew, client_t* client, http_t* http)
{
    if (http->cookies.remember_token == NULL)
    {
        server_http_resp_error(client, HTTP_CODE_UNAUTHORIZED, "No remember_token cookie");
        return RECV_DISCONNECT;
    }
    else if (strlen(http->cookies.remember_token) != TOKEN_HEX_LEN)
    {
        server_http_resp(client, HTTP_CODE_UNAUTHORIZED);
        return RECV_DISCONNECT;
    }
    u8 token[TOKEN_LEN];
    u8 token_hash[TOKEN_LEN];

    // Convert token hex string into binary.
    hexstr_to_u8(http->cookies.remember_token, TOKEN_HEX_LEN, token);

    // SHA-256 binary token.
    server_sha256(token, TOKEN_LEN, token_hash);

    if (server_auth_token_get_rotate(ew, client, token_hash, after_token_create) == false)
    {
        server_http_resp(client, HTTP_CODE_INTERAL_ERROR);
        return RECV_ERROR;
    }

    return RECV_OK;
}

/**
 *  Called after selecting user from database.
 *  Hashes the given password with SHA512 and compares to stored hash.
 *  If they match, calls `async_create_session()`.
 */
static const char*
do_client_login(eworker_t* ew, dbcmd_ctx_t* ctx)
{
    dbuser_t* user = ctx->data;
    const char* password = ctx->param.user_login.password;
    const char* username = ctx->param.user_login.username;
    client_t* client = ctx->client;
    u8 hash_login[SERVER_HASH_SIZE];
    bool remember_me = ctx->param.user_login.remember_me;
    bool successful = false;

    if (ctx->ret == DB_ASYNC_ERROR)
    {
        server_http_resp_error(client, HTTP_CODE_UNAUTHORIZED, ERR_MSG_INCORRECT);
        goto log_login_attempt;
    }

    server_sha512(password, user->salt, hash_login);

    if (CRYPTO_memcmp(user->hash, hash_login, SERVER_HASH_SIZE) == 0)
    {
        server_auth_token_create(ew, user->user_id, client, remember_me, after_token_create);
        successful = true;
    }
    else
        server_http_resp_error(client, HTTP_CODE_UNAUTHORIZED, ERR_MSG_INCORRECT);

log_login_attempt:
    db_async_login_attempt(&ew->db, username, successful, client->user_agent, client->addr.ip_str);
    return NULL;
}

/**
 *  Asynchronously retrieves user by username from database.
 *  Then calls `do_client_login()`.
 */
static inline enum client_recv_status
server_handle_auth_login(eworker_t* ew, 
                         client_t* client, 
                         const char* username, 
                         const char* password, 
                         const bool remember_me)
{
    dbcmd_ctx_t ctx = {
        .exec = do_client_login,
        .param.user_login.remember_me = remember_me
    };
    strncpy(ctx.param.user_login.password, password, DB_PASSWORD_MAX - 1);
    strncpy(ctx.param.user_login.username, username, DB_USERNAME_MAX - 1);

    if (!db_async_get_user_username(&ew->db, username, &ctx))
    {
        server_http_resp(client, HTTP_CODE_INTERAL_ERROR);
        return RECV_ERROR;
    }

    return RECV_OK;
}

/**
 *  Called after inserting user into database.
 *  If successful, call `async_create_session()`, otherwise respond with HTTP Conflict.
 */
static const char* 
do_client_register(eworker_t* ew, dbcmd_ctx_t* ctx)
{
    client_t* client = ctx->client;
    dbuser_t* user = ctx->data;
    bool remember_me = ctx->param.user_login.remember_me;

    if (ctx->ret == DB_ASYNC_ERROR)
    {
        server_http_resp_error(client, HTTP_CODE_CONFLICT, "Username already taken");
        return NULL;
    }

    server_auth_token_create(ew, user->user_id, client, remember_me, after_token_create);

    return NULL;
}

/**
 * Asynchronously inserts user to database.
 * Then calls `do_client_register()`.
 */
static inline enum client_recv_status
server_handle_auth_register(eworker_t* ew, 
                            client_t* client, 
                            const char* username,
                            const char* displayname, 
                            const char* password,
                            const bool remember_me)
{
    dbuser_t* new_user;

    new_user = server_new_user(ew, 0);
    strncpy(new_user->username, username, DB_USERNAME_MAX - 1);
    strncpy(new_user->displayname, displayname, DB_DISPLAYNAME_MAX - 1);
    getrandom(new_user->salt, SERVER_SALT_SIZE, 0);
    server_sha512(password, new_user->salt, new_user->hash);

    dbcmd_ctx_t ctx = {
        .exec = do_client_register,
        .param.user_login.remember_me = remember_me
    };
    if (!db_async_insert_user(&ew->db, new_user, &ctx))
    {
        server_http_resp(client, HTTP_CODE_INTERAL_ERROR);
        return RECV_ERROR;
    }

    return RECV_OK;
}

static inline enum client_recv_status
handle_post(eworker_t* ew, client_t* client, http_t* http)
{
    json_object* payload;
    json_object* json_username;
    json_object* json_displayname;
    json_object* json_password;
    json_object* json_remember_me;
    const char* username;
    const char* password;
    const char* displayname;
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

    if (strcmp(http->req.url, "/api/auth/register") == 0)
    {
        /**
        * Check for "displayname" key and type string 
        **/
        json_displayname = json_object_object_get(payload, "displayname");
        if (json_object_is_type(json_displayname, json_type_string) == 0)
            goto bad_req;
        displayname = json_object_get_string(json_displayname);

        ret = server_handle_auth_register(ew, client, username, displayname, password, remember_me);
        goto free_payload;
    }

    server_http_resp(client, HTTP_CODE_NOT_FOUND);
    json_object_put(payload);
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
        if (strcmp(http->req.url, "/api/auth/remember") == 0)
            return server_handle_auth_remember(ew, client, http);
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

static void 
bind_user_to_client(eworker_t* ew, client_t* client, dbuser_t* user)
{
    server_ght_insert(&ew->server->user_ht, user->user_id, user);
    server_rtusm_user_connect(ew, user);
    array_add_voidp(&user->connected_clients, client);
    client->dbuser = user;
    client->state |= CLIENT_STATE_LOGGED_IN;

    info("Login: IP=[%s], Agent='%s', ID=%u, Username='%s', DisplayName='%s'\n",
         client->addr.ip_str, client->user_agent, 
         user->user_id, user->username, user->displayname);

    if ((client->state & CLIENT_STATE_WEBSOCKET) == 0)
        server_http_switch_to_websocket(client);
}

static const char* 
after_get_user(eworker_t* ew, dbcmd_ctx_t* ctx)
{
    client_t* client = ctx->client;
    dbuser_t* user = ctx->data;

    if (ctx->ret == DB_ASYNC_ERROR)
    {
        server_http_resp_error(client, HTTP_CODE_NOT_FOUND, "User not found");
        return NULL;
    }

    bind_user_to_client(ew, client, user);

    return NULL;
}

static inline void 
get_bind_user_to_client(eworker_t* ew, client_t* client, u32 user_id)
{
    dbcmd_ctx_t ctx = {
        .client = client,
        .exec = after_get_user,
        .flags = DB_CTX_DONT_FREE
    };

    if (!db_async_get_user(&ew->db, user_id, &ctx))
        server_http_resp(client, HTTP_CODE_INTERAL_ERROR);
}

static void 
after_get_session(eworker_t* ew, get_session_data_t* data)
{
    client_t* client = data->client;
    u32 user_id = data->user_id;
    dbuser_t* user;

    if (data->found == false)
    {
        server_http_resp(client, HTTP_CODE_UNAUTHORIZED);
        return;
    }

    user = server_ght_get(&ew->server->user_ht, user_id);
    if (user)
        bind_user_to_client(ew, client, user);
    else
        get_bind_user_to_client(ew, client, user_id);
}

enum client_recv_status
server_auth_websocket_upgrade(eworker_t* ew, client_t* client, http_t* http)
{
    redis_cb_data_t* data;
    const char* session;

    if ((session = http->cookies.session_uuid) == NULL)
    {
        server_http_resp(client, HTTP_CODE_UNAUTHORIZED);
        return RECV_DISCONNECT;
    }

    data = server_redis_get_cb_data(&ew->redis);
    data->data.http = http;
    data->data.client = client;
    data->data.user_id = 0;
    data->callback = after_get_session;
    server_redis_get_session(&ew->redis, session, data);

    return RECV_OK;
}
