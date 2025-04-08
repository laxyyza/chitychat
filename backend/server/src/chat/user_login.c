#include "chat/user_login.h"
#include "chat/db.h"
#include "chat/db_def.h"
#include "chat/db_user.h"
#include "chat/rtusm.h"
#include "chat/db_user_session.h"
#include "chat/user_session.h"
#include "chat/ws_text_frame.h"
#include "server_ht.h"
#include "server_http.h"

#define INCORRECT_LOGIN_STR "Incorrect Username or Password"
#define U64_STR_LEN 21

static const char* 
server_set_client_logged_in(eworker_t* ew, 
                            client_t* client, 
                            dbuser_t* user,
                            dbsession_t* session, 
                            json_object* respond_json)
{
    const char* errmsg;
    char token[U64_STR_LEN];

    if (!(client->state & CLIENT_STATE_WEBSOCKET))
        server_http_switch_to_websocket(client);

    if (session)
    {
        client->state |= CLIENT_STATE_SESSION_PENDING;
        getrandom(&client->tmptoken, sizeof(u64), 0);
        server_ght_insert(&ew->server->client_by_tmptoken_ht, client->tmptoken, client);
        server_ght_insert(&ew->server->client_by_session_ht, server_ght_hash_uuid(session->uuid), client);
        strncpy(client->session_uuid, session->uuid, UUID_LEN - 1);
    }
    else
        client->tmptoken = 0;

    if ((errmsg = server_user_rate_limit_check(user)))
        return errmsg; 

    server_ght_insert(&ew->server->user_ht, user->user_id, user);
    server_rtusm_user_connect(ew, user);
    array_add_voidp(&user->connected_clients, client);
    client->dbuser = user;

    snprintf(token, U64_STR_LEN, "%lu", client->tmptoken);

    json_object_object_add(respond_json, "cmd", 
                        json_object_new_string("session"));
    json_object_object_add(respond_json, "id", 
                           json_object_new_string(token));

    if (ws_json_send(client, respond_json) != -1)
    {
        client->state |= CLIENT_STATE_LOGGED_IN;

        info("Client (IP: %s) login as user:\n\t\t{ id: %u, username: '%s', displayname: '%s'}\n",
            client->addr.ip_str, user->user_id, user->username, user->displayname);
    }

    return NULL;
}

static const char*
do_client_login_session(eworker_t* ew, dbcmd_ctx_t* ctx)
{
    dbsession_t* session = ctx->param.session;
    dbuser_t* user = ctx->data;
    const char* errmsg;

    if (ctx->ret == DB_ASYNC_ERROR)
        return "Could not find user from session";

    json_object* resp = json_object_new_object();
    errmsg = server_set_client_logged_in(ew, ctx->client, user, session, resp);
    json_object_put(resp);

    // IDK if session needs to be freed or not.

    if (errmsg == NULL)
        ctx->data = NULL;

    return errmsg;
}

static const char* 
do_get_session(eworker_t* ew, dbcmd_ctx_t* ctx)
{
    dbsession_t* session  = ctx->data;
    dbuser_t* user;
    const char* errmsg = NULL;
    ctx->data = NULL;

    if (ctx->ret == DB_ASYNC_ERROR)
    {
        server_http_resp(ctx->client, HTTP_CODE_UNAUTHORIZED);
        return "Invalid session ID";
    }

    ctx->param.session = session;

    if ((user = server_ght_get(&ew->server->user_ht, session->user_id)))
    {
        ctx->data = user;
        errmsg = do_client_login_session(ew, ctx);
        ctx->data = NULL;
    }
    else
    {
        ctx->exec = do_client_login_session;

        if (!db_async_get_user(&ew->db, session->user_id, ctx))
            return "Internal error: async-get-user";
    }

    return errmsg;
}

/**
 *  `ew->db.ctx.client` will contain the client reference. 
 *  No need to pass client_t in this function.
 **/
const char* 
server_client_login_session_uuid(eworker_t* ew, 
                                 const char* session_uuid)
{
    dbsession_t* session;
    
    dbcmd_ctx_t ctx = {
        .exec = do_get_session,
    };

    session = calloc(1, sizeof(dbsession_t));
    strncpy(session->uuid, session_uuid, UUID_LEN - 1);

    if (!db_async_select_session(&ew->db, session, &ctx))
        return "Internal error: async-select-session";

    return NULL;
}

// static const char* 
// after_session_insert(eworker_t* ew, dbcmd_ctx_t* ctx)
// {
//     const char* errmsg;
//     json_object* resp;
//     client_t* client;
//     dbuser_t* user;
//     dbsession_t* session;
//
//     if (ctx->ret == DB_ASYNC_ERROR)
//         return "Failed to create session";
//
//     client = ctx->param.session_login.client;
//     user = ctx->param.session_login.user;
//     session = ctx->data;
//
//     resp = json_object_new_object();
//     errmsg = server_set_client_logged_in(ew, client, user, session, resp);
//     json_object_put(resp);
//
//     return errmsg;
// }

// static const char*
// async_create_session(eworker_t* ew, client_t* client, dbuser_t* user, dbsession_t* session)
// {
//     dbcmd_ctx_t ctx = {
//         .exec = after_session_insert,
//         .param.session_login.client = client,
//         .param.session_login.user = user,
//         .client = client
//     };
//     if (!db_async_insert_session(&ew->db, user->user_id, &ctx))
//     {
//         free(session);
//         return "Internal error: async-insert-session";
//     }
//
//     return NULL;
// }
//
// static const char*
// do_client_login(eworker_t* ew, dbcmd_ctx_t* ctx)
// {
//     const char* errmsg = NULL;
//     dbuser_t* user = ctx->data;
//     const char* password = ctx->param.user_login.password;
//     // const bool do_session = ctx->param.user_login.do_session;
//     dbsession_t* session;
//     u8 hash_login[SERVER_HASH_SIZE];
//
//     if (ctx->ret == DB_ASYNC_ERROR)
//         return INCORRECT_LOGIN_STR;
//     if (user == NULL)
//     {
//         error("db_client_login: ctx->data is NULL!\n");
//         return "Internal error: ctx->data";
//     }
//
//     server_sha512(password, user->salt, hash_login);
//
//     if (memcmp(user->hash, hash_login, SERVER_HASH_SIZE) == 0)
//     {
//         // if (do_session)
//         // {
//         session = calloc(1, sizeof(dbsession_t));
//         session->user_id = user->user_id;
//         errmsg = async_create_session(ew, ctx->client, user, session);
//         // }
//         // else
//         // {
//         //     json_object* resp = json_object_new_object();
//         //     errmsg = server_set_client_logged_in(ew, ctx->client, user, NULL, resp);
//         //     json_object_put(resp);
//         // }
//     }
//     else
//         errmsg = INCORRECT_LOGIN_STR;
//
//     /*
//      * Set ctx->data to NULL if client logged-in.
//      * If client logged-in: client->dbuser = ctx->data.
//      * After this function ctx->data will be freed.
//      */
//     if (errmsg == NULL)
//         ctx->data = NULL;
//
//     return errmsg;
// }

// const char* 
// server_client_login(eworker_t* ew, 
//                     UNUSED client_t* client, 
//                     json_object* payload, 
//                     UNUSED json_object* respond_json)
// {
//     json_object* username_json;
//     json_object* password_json;
//     json_object* do_session_json;
//     const char* username;
//     const char* password;
//     // dbuser_t* user;
//     // session_t* session;
//     bool do_session;
//     // u8 hash_login[SERVER_HASH_SIZE];
//     // u8* salt;
//     // const char* errmsg = NULL;
//
//     RET_IF_JSON_BAD(username_json, payload, "username", json_type_string);
//     RET_IF_JSON_BAD(password_json, payload, "password", json_type_string);
//     RET_IF_JSON_BAD(do_session_json, payload, "session", json_type_boolean);
//
//     username = json_object_get_string(username_json);
//     password = json_object_get_string(password_json);
//     do_session = json_object_get_boolean(do_session_json);
//
//     dbcmd_ctx_t ctx;
//     memset(&ctx, 0, sizeof(dbcmd_ctx_t));
//
//     ctx.exec = do_client_login;
//     ctx.param.user_login.do_session = do_session;
//     strncpy(ctx.param.user_login.password, password, DB_PASSWORD_MAX - 1);
//
//     if (!db_async_get_user_username(&ew->db, username, &ctx))
//         return "Failed to do async sql.\n";
//     return NULL;
// }
//
// static const char* 
// do_client_register(eworker_t* ew, dbcmd_ctx_t* ctx)
// {
//     const char* errmsg = NULL;
//     dbuser_t* user = ctx->data;
//     dbsession_t* session;
//     const bool do_session = ctx->param.user_login.do_session;
//
//     if (ctx->ret == DB_ASYNC_ERROR)
//         return "Username already taken";
//
//     if (do_session)
//     {
//         session = calloc(1, sizeof(dbsession_t));
//         session->user_id = user->user_id;
//     }
//     else 
//         session = NULL;
//
//     if (session)
//         errmsg = async_create_session(ew, ctx->client, user, session);
//     else
//     {
//         json_object* resp = json_object_new_object();
//         errmsg = server_set_client_logged_in(ew, ctx->client, user, session, resp);
//         json_object_put(resp);
//     }
//
//     /* See user_login.c:do_client_login() why setting ctx->data to NULL */
//     if (errmsg == NULL)
//         ctx->data = NULL;
//
//     return errmsg;
// }

// const char* 
// server_client_register(eworker_t* ew, 
//                        UNUSED client_t* client, 
//                        json_object* payload,
//                        UNUSED json_object* resp)
// {
//     json_object* username_json;
//     json_object* displayname_json;
//     json_object* password_json;
//     json_object* do_session_json;
//     const char* username;
//     const char* displayname;
//     const char* password;
//     bool do_session;
//     dbuser_t* new_user;
//     const char* errmsg = NULL;
//
//     RET_IF_JSON_BAD(username_json,      payload, "username",    json_type_string);
//     RET_IF_JSON_BAD(displayname_json,   payload, "displayname", json_type_string);
//     RET_IF_JSON_BAD(password_json,      payload, "password",    json_type_string);
//     RET_IF_JSON_BAD(do_session_json,    payload, "session",     json_type_boolean);
//
//     username = json_object_get_string(username_json);
//     displayname = json_object_get_string(displayname_json);
//     password = json_object_get_string(password_json);
//     do_session = json_object_get_boolean(do_session_json);
//
//     new_user = server_new_user(ew, 0);
//     strncpy(new_user->username, username, DB_USERNAME_MAX - 1);
//     strncpy(new_user->displayname, displayname, DB_USERNAME_MAX - 1);
//
//     getrandom(new_user->salt, SERVER_SALT_SIZE, 0);
//     server_sha512(password, new_user->salt, new_user->hash);
//
//     dbcmd_ctx_t ctx = {
//         .param.user_login.do_session = do_session,
//         .exec = do_client_register,
//     };
//     if (db_async_insert_user(&ew->db, new_user, &ctx) == false)
//     {
//         errmsg = "Internal error: async-insert-user";
//         free(new_user);
//     }
//     return errmsg;
// }
