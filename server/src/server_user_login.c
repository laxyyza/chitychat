#include "server_user_login.h"
#include "server.h"
#include "server_client_sesson.h"
#include "server_events.h"
#include "server_tm.h"
#include "server_ws_pld_hdlr.h"
#include <json-c/json_object.h>

static const char* 
server_set_client_logged_in(server_thread_t* th, client_t* client, 
                session_t* session, json_object* respond_json)
{
    dbuser_t* dbuser;
    server_event_t* session_timer_ev = NULL;

    dbuser = server_db_get_user_from_id(&th->db, session->user_id);
    if (!dbuser)
    {
        server_del_client_session(th->server, session);
        return "Server could not find user in database";
    }
    memcpy(client->dbuser, dbuser, sizeof(dbuser_t));
    free(dbuser);

    json_object_object_add(respond_json, "type", 
                        json_object_new_string("session"));
    json_object_object_add(respond_json, "id", 
                        json_object_new_uint64(session->session_id));

    if (ws_json_send(client, respond_json) != -1)
    {
        client->state |= CLIENT_STATE_LOGGED_IN;
        client->session = session;
    }

    if (session->timerfd)
    {
        session_timer_ev = server_get_event(th->server, session->timerfd);
        if (session_timer_ev)
            server_del_event(th->server, session_timer_ev);
        session->timerfd = 0;
    }

    return NULL;
}

static const char* 
server_handle_client_session(server_thread_t* th, client_t* client, 
                json_object* payload, json_object* respond_json)
{
    json_object* session_id_json;
    session_t* session;
    u32 session_id;
    
    RET_IF_JSON_BAD(session_id_json, payload, "id", json_type_int);
    session_id = json_object_get_uint64(session_id_json);

    session = server_get_client_session(th->server, session_id);
    if (!session)
        return "Invalid session ID or session expired";

    return server_set_client_logged_in(th, client, session, respond_json);
}

static const char* 
server_handle_client_login(server_thread_t* th, const char* username, 
                                const char* password)
{
    dbuser_t* user = server_db_get_user_from_name(&th->db, username);
    if (!user)
        goto error;

    u8 hash_login[SERVER_HASH_SIZE];
    u8* salt = user->salt;
    server_sha512(password, salt, hash_login);

    if (memcmp(user->hash, hash_login, SERVER_HASH_SIZE) != 0)
        goto error;

    debug("\tUser:%u, %s '%s' logged in.\n", 
            user->user_id, user->username, user->displayname);

    free(user);

    return NULL;
error:
    if (user)
        free(user);
    return "Incorrect Username or Password";
}

static const char* 
server_handle_client_register(server_thread_t* th, 
                              const char* username, 
                              const char* displayname, 
                              const char* password)
{
    if (!displayname)
        return "Require display name.";

    dbuser_t new_user;
    memset(&new_user, 0, sizeof(dbuser_t));
    strncpy(new_user.username, username, DB_USERNAME_MAX);
    strncpy(new_user.displayname, displayname, DB_USERNAME_MAX);

    getrandom(new_user.salt, SERVER_SALT_SIZE, 0);
    server_sha512(password, new_user.salt, new_user.hash);

    if (!server_db_insert_user(&th->db, &new_user))
        return "Username already taken";

    info("\tUser created: '%s' (%s)\n", new_user.displayname, new_user.username);

    return NULL;
}

static const char* 
server_create_client_session(server_thread_t* th, client_t* client, 
                const char* username, json_object* respond_json)
{
    session_t* session;

    dbuser_t* user = server_db_get_user_from_name(&th->db, username);
    if (!user)
        return "Invalid username";

    session = server_new_client_session(th->server, client);
    session->user_id = user->user_id;
    free(user);

    return server_set_client_logged_in(th, client, session, respond_json);
}

const char* 
server_handle_not_logged_in_client(server_thread_t* th, client_t* client, 
        json_object* payload, json_object* respond_json, const char* type)
{
    json_object* username_json;
    json_object* password_json;
    json_object* displayname_json;
    const char* username;
    const char* password;
    const char* displayname;
    const char* errmsg = NULL;

    if (!strcmp(type, "session"))
        return server_handle_client_session(th, client, payload, 
                                            respond_json);

    RET_IF_JSON_BAD(username_json, payload, "username", json_type_string);
    RET_IF_JSON_BAD(password_json, payload, "password", json_type_string);

    displayname_json = json_object_object_get(payload, "displayname");
    if (displayname_json && json_object_is_type(displayname_json, json_type_string))
        return JSON_INVALID_STR("displayname");

    username = json_object_get_string(username_json);
    password = json_object_get_string(password_json);
    displayname = json_object_get_string(displayname_json);

    if (!strcmp(type, "login"))
        errmsg = server_handle_client_login(th, username, password);
    else if (!strcmp(type, "register"))
        errmsg = server_handle_client_register(th, username, 
                                               displayname, password);
    else
        return "Not logged in.";

    if (!errmsg)
        server_create_client_session(th, client, username, respond_json);
    return errmsg;
}
