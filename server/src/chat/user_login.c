#include "chat/user_login.h"
#include "chat/rtusm.h"
#include "chat/user_session.h"
#include "chat/ws_text_frame.h"

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

const char* 
server_client_login_session(eworker_t* ew, 
                            client_t* client, 
                            json_object* payload, 
                            json_object* respond_json)
{
    json_object* session_id_json;
    session_t* session;
    dbuser_t* user;
    u32 session_id;
    
    RET_IF_JSON_BAD(session_id_json, payload, "id", json_type_int);
    session_id = json_object_get_uint64(session_id_json);

    session = server_get_user_session(ew->server, session_id);
    if (!session)
        return "Invalid session ID or session expired";

    user = server_db_get_user_from_id(&ew->db, session->user_id);
    if (!user)
    {
        server_del_user_session(ew->server, session);
        return "Could not find user from session";
    }

    return server_set_client_logged_in(ew, client, user, session, respond_json);
}

const char* 
server_client_login(eworker_t* ew, 
                    client_t* client, 
                    json_object* payload, 
                    json_object* respond_json)
{
    json_object* username_json;
    json_object* password_json;
    json_object* do_session_json;
    const char* username;
    const char* password;
    dbuser_t* user;
    session_t* session;
    bool do_session;
    u8 hash_login[SERVER_HASH_SIZE];
    u8* salt;
    const char* errmsg = NULL;

    RET_IF_JSON_BAD(username_json, payload, "username", json_type_string);
    RET_IF_JSON_BAD(password_json, payload, "password", json_type_string);
    RET_IF_JSON_BAD(do_session_json, payload, "session", json_type_boolean);

    username = json_object_get_string(username_json);
    password = json_object_get_string(password_json);
    do_session = json_object_get_boolean(do_session_json);

    user = server_db_get_user_from_name(&ew->db, username);
    if (!user)
        return INCORRECT_LOGIN_STR;

    salt = user->salt;
    server_sha512(password, salt, hash_login);

    if (memcmp(user->hash, hash_login, SERVER_HASH_SIZE) == 0)
    {
        if (do_session)
        {
            session = server_new_user_session(ew->server, client);
            session->user_id = user->user_id;
        }
        else 
            session = NULL;

        errmsg = server_set_client_logged_in(ew, client, user, session, respond_json);
    }
    else
        errmsg = INCORRECT_LOGIN_STR;

    if (errmsg)
        free(user);
    return errmsg;
}

const char* 
server_client_register(eworker_t* ew, 
                       client_t* client, 
                       json_object* payload,
                       json_object* resp)
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
    session_t* session = NULL;

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

    if (!server_db_insert_user(&ew->db, new_user))
    {
        free(new_user);
        return "Username already taken";
    }

    info("\tUser created: { id: %u, username: '%s', displayname: '%s'}\n", 
         new_user->user_id, new_user->displayname, new_user->username);

    if (do_session)
    {
        session = server_new_user_session(ew->server, client);
        session->user_id = new_user->user_id;
    }

    if ((errmsg = server_set_client_logged_in(ew, client, new_user, session, resp)))
        free(new_user);

    return errmsg;
}
