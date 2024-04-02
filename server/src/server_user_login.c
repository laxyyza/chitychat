#include "server_user_login.h"
#include "server.h"
#include "server_client_sesson.h"
#include "server_events.h"

static const char* 
server_set_client_logged_in(server_t* server, client_t* client, 
                session_t* session, json_object* respond_json)
{
    dbuser_t* dbuser;
    server_event_t* session_timer_ev = NULL;

    dbuser = server_db_get_user_from_id(server, session->user_id);
    if (!dbuser)
    {
        server_del_client_session(server, session);
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
        session_timer_ev = server_get_event(server, session->timerfd);
        if (session_timer_ev)
            server_del_event(server, session_timer_ev);
        session->timerfd = 0;
    }

    return NULL;
}

static const char* 
server_handle_client_session(server_t* server, client_t* client, 
                json_object* payload, json_object* respond_json)
{
    json_object* session_id_json;
    session_t* session;
    u32 session_id;

    session_id_json = json_object_object_get(payload, "id");
    session_id = json_object_get_uint64(session_id_json);

    session = server_get_client_session(server, session_id);
    if (!session)
        return "Invalid session ID or session expired";

    return server_set_client_logged_in(server, client, session, respond_json);
}

static const char* 
server_handle_client_login(server_t* server, const char* username, 
                                const char* password)
{
    dbuser_t* user = server_db_get_user_from_name(server, username);
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
server_handle_client_register(server_t* server, 
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

    if (!server_db_insert_user(server, &new_user))
        return "Username already taken";

    info("\tUser created: '%s' (%s)\n", new_user.displayname, new_user.username);

    return NULL;
}

static const char* 
server_create_client_session(server_t* server, client_t* client, 
                const char* username, json_object* respond_json)
{
    session_t* session;

    dbuser_t* user = server_db_get_user_from_name(server, username);
    if (!user)
        return "Invalid username";

    session = server_new_client_session(server, client);
    session->user_id = user->user_id;
    free(user);

    return server_set_client_logged_in(server, client, session, respond_json);
}

const char* 
server_handle_not_logged_in_client(server_t* server, client_t* client, 
        json_object* payload, json_object* respond_json, const char* type)
{
    json_object* username_json = json_object_object_get(payload, 
            "username");
    json_object* password_json = json_object_object_get(payload, 
            "password");
    json_object* displayname_json = json_object_object_get(payload, 
            "displayname");

    if (!strcmp(type, "session"))
        return server_handle_client_session(server, client, payload, 
                                            respond_json);

    const char* username = json_object_get_string(username_json);
    const char* password = json_object_get_string(password_json);
    const char* displayname = json_object_get_string(displayname_json);

    if (!username_json || !username)
        return "Require username.";
    if (!password_json || !password)
        return "Require password.";

    const char* errmsg = NULL;

    if (!strcmp(type, "login"))
        errmsg = server_handle_client_login(server, username, password);
    else if (!strcmp(type, "register"))
        errmsg = server_handle_client_register(server, username, 
                                               displayname, password);
    else
        return "Not logged in.";

    if (!errmsg)
        server_create_client_session(server, client, username, respond_json);
    return errmsg;
}
