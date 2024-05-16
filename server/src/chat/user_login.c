#include "chat/user_login.h"
#include "chat/db.h"
#include "chat/db_def.h"
#include "chat/db_user.h"
#include "chat/rtusm.h"
#include "chat/user_session.h"
#include "chat/ws_text_frame.h"
#include "server_ht.h"

#define INCORRECT_LOGIN_STR "Incorrect Username or Password"

static const char* 
server_set_client_logged_in(eworker_t* ew, 
                            client_t* client, 
                            dbuser_t* user,
                            session_t* session, 
                            json_object* respond_json)
{
    server_event_t* session_timer_ev = NULL;
    const client_t* client_already_logged_in;
    const u64 session_id = (session) ? session->session_id : 0;

    client_already_logged_in = server_get_client_user_id(ew->server, user->user_id);
    if (client_already_logged_in)
        return "Someone else already logged in";

    client->dbuser = user;
    server_ght_insert(&ew->server->user_ht, user->user_id, client);

    json_object_object_add(respond_json, "cmd", 
                        json_object_new_string("session"));
    json_object_object_add(respond_json, "id", 
                           json_object_new_uint64(session_id));

    if (ws_json_send(client, respond_json) != -1)
    {
        client->state |= CLIENT_STATE_LOGGED_IN;
        client->session = session;

        debug("Client (fd: %d, IP: %s) logged as user:\n\t\t{ id: %u, username: '%s', displayname: '%s'}\n",
            client->addr.sock, client->addr.ip_str, 
            user->user_id, user->username, user->displayname);
    }

    if (session && session->timerfd)
    {
        session_timer_ev = server_get_event(ew->server, session->timerfd);
        if (session_timer_ev)
        {
            session_timer_ev->keep_data = true;
            server_del_event(ew, session_timer_ev);
        }
        session->timerfd = 0;
    }

    return NULL;
}

static const char*
do_client_login_session(eworker_t* ew, dbcmd_ctx_t* ctx)
{
    session_t* session = ctx->param.session;
    dbuser_t* user = ctx->data;
    const char* errmsg;

    if (ctx->ret == DB_ASYNC_ERROR)
    {
        server_del_user_session(ew->server, ctx->param.session);
        return "Could not find user from session";
    }

    json_object* resp = json_object_new_object();
    errmsg = server_set_client_logged_in(ew, ctx->client, user, session, resp);
    json_object_put(resp);

    if (errmsg == NULL)
        ctx->data = NULL;

    return errmsg;
}

const char* 
server_client_login_session(eworker_t* ew, 
                            UNUSED client_t* client, 
                            json_object* payload, 
                            UNUSED json_object* respond_json)
{
    json_object* session_id_json;
    session_t* session;
    u32 session_id;
    
    RET_IF_JSON_BAD(session_id_json, payload, "id", json_type_int);
    session_id = json_object_get_uint64(session_id_json);

    session = server_get_user_session(ew->server, session_id);
    if (!session)
        return "Invalid session ID or session expired";

    dbcmd_ctx_t ctx = {
        .exec = do_client_login_session,
        .param.session = session
    };
    if (db_async_get_user(&ew->db, session->user_id, &ctx) == false)
        return "Internal error: async-get-user";
    return NULL;
}

static const char*
do_client_login(eworker_t* ew, dbcmd_ctx_t* ctx)
{
    const char* errmsg = NULL;
    dbuser_t* user = ctx->data;
    const char* password = ctx->param.user_login.password;
    const bool do_session = ctx->param.user_login.do_session;
    session_t* session;
    u8 hash_login[SERVER_HASH_SIZE];

    if (ctx->ret == DB_ASYNC_ERROR)
        return INCORRECT_LOGIN_STR;
    if (user == NULL)
    {
        error("db_client_login: ctx->data is NULL!\n");
        return "Internal error: ctx->data";
    }

    server_sha512(password, user->salt, hash_login);

    if (memcmp(user->hash, hash_login, SERVER_HASH_SIZE) == 0)
    {
        if (do_session)
        {
            session = server_new_user_session(ew->server, ctx->client);
            session->user_id = user->user_id;
        }
        else
            session = NULL;

        json_object* resp = json_object_new_object();
        errmsg = server_set_client_logged_in(ew, ctx->client, user, session, resp);
        json_object_put(resp);
    }
    else
        errmsg = INCORRECT_LOGIN_STR;

    /*
     * Set ctx->data to NULL if client logged-in.
     * If client logged-in: client->dbuser = ctx->data.
     * After this function ctx->data will be freed.
     */
    if (errmsg == NULL)
        ctx->data = NULL;

    return errmsg;
}

const char* 
server_client_login(eworker_t* ew, 
                    UNUSED client_t* client, 
                    json_object* payload, 
                    UNUSED json_object* respond_json)
{
    json_object* username_json;
    json_object* password_json;
    json_object* do_session_json;
    const char* username;
    const char* password;
    // dbuser_t* user;
    // session_t* session;
    bool do_session;
    // u8 hash_login[SERVER_HASH_SIZE];
    // u8* salt;
    // const char* errmsg = NULL;

    RET_IF_JSON_BAD(username_json, payload, "username", json_type_string);
    RET_IF_JSON_BAD(password_json, payload, "password", json_type_string);
    RET_IF_JSON_BAD(do_session_json, payload, "session", json_type_boolean);

    username = json_object_get_string(username_json);
    password = json_object_get_string(password_json);
    do_session = json_object_get_boolean(do_session_json);

    dbcmd_ctx_t ctx;
    memset(&ctx, 0, sizeof(dbcmd_ctx_t));

    ctx.exec = do_client_login;
    ctx.param.user_login.do_session = do_session;
    strncpy(ctx.param.user_login.password, password, DB_PASSWORD_MAX);

    if (!db_async_get_user_username(&ew->db, username, &ctx))
        return "Failed to do async sql.\n";
    return NULL;
}

static const char* 
do_client_register(eworker_t* ew, dbcmd_ctx_t* ctx)
{
    const char* errmsg = NULL;
    dbuser_t* user = ctx->data;
    session_t* session;
    const bool do_session = ctx->param.user_login.do_session;

    if (ctx->ret == DB_ASYNC_ERROR)
        return "Username already taken";

    if (do_session)
    {
        session = server_new_user_session(ew->server, ctx->client);
        session->user_id = user->user_id;
    }
    else 
        session = NULL;

    json_object* resp = json_object_new_object();
    errmsg = server_set_client_logged_in(ew, ctx->client, user, session, resp);
    json_object_put(resp);

    /* See user_login.c:do_client_login() why setting ctx->data to NULL */
    if (errmsg == NULL)
        ctx->data = NULL;

    return errmsg;
}

const char* 
server_client_register(eworker_t* ew, 
                       UNUSED client_t* client, 
                       json_object* payload,
                       UNUSED json_object* resp)
{
    json_object* username_json;
    json_object* displayname_json;
    json_object* password_json;
    json_object* do_session_json;
    const char* username;
    const char* displayname;
    const char* password;
    bool do_session;
    dbuser_t* new_user;
    const char* errmsg = NULL;

    RET_IF_JSON_BAD(username_json,      payload, "username",    json_type_string);
    RET_IF_JSON_BAD(displayname_json,   payload, "displayname", json_type_string);
    RET_IF_JSON_BAD(password_json,      payload, "password",    json_type_string);
    RET_IF_JSON_BAD(do_session_json,    payload, "session",     json_type_boolean);

    username = json_object_get_string(username_json);
    displayname = json_object_get_string(displayname_json);
    password = json_object_get_string(password_json);
    do_session = json_object_get_boolean(do_session_json);

    new_user = calloc(1, sizeof(dbuser_t));
    strncpy(new_user->username, username, DB_USERNAME_MAX);
    strncpy(new_user->displayname, displayname, DB_USERNAME_MAX);

    getrandom(new_user->salt, SERVER_SALT_SIZE, 0);
    server_sha512(password, new_user->salt, new_user->hash);

    dbcmd_ctx_t ctx = {
        .param.user_login.do_session = do_session,
        .exec = do_client_register,
    };
    if (db_async_insert_user(&ew->db, new_user, &ctx) == false)
    {
        errmsg = "Internal error: async-insert-user";
        free(new_user);
    }
    return errmsg;
}
