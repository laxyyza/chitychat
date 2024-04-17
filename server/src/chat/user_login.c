#include "chat/user_login.h"
#include "chat/rtusm.h"
#include "chat/ws_text_frame.h"

static void 
notify_client_login_attempt(const client_t* client, const client_t* login_attempt_client)
{
    json_object* resp_json = json_object_new_object();

    json_object_object_add(resp_json, "cmd", 
                           json_object_new_string("warn_login_attempt"));
    json_object_object_add(resp_json, "msg",
                           json_object_new_string("Someone tried to login into your account"));
    json_object_object_add(resp_json, "ip",
                           json_object_new_string(login_attempt_client->addr.ip_str));
    
    ws_json_send(client, resp_json);

    json_object_put(resp_json);
}

static const char* 
server_set_client_logged_in(server_thread_t* th, 
                            client_t* client, 
                            dbuser_t* user,
                            session_t* session, 
                            json_object* respond_json)
{
    server_event_t* session_timer_ev = NULL;
    const client_t* client_already_logged_in;

    client_already_logged_in = server_get_client_user_id(th->server, session->user_id);
    if (client_already_logged_in)
    {
        notify_client_login_attempt(client_already_logged_in, client);
        return "Someone else already logged in";
    }

    client->dbuser = user;

    json_object_object_add(respond_json, "cmd", 
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
        {
            session_timer_ev->keep_data = true;
            server_del_event(th, session_timer_ev);
        }
        session->timerfd = 0;
    }

    return NULL;
}

static const char* 
server_handle_user_session(server_thread_t* th, client_t* client, 
                             json_object* payload, json_object* respond_json)
{
    json_object* session_id_json;
    session_t* session;
    dbuser_t* user;
    u32 session_id;
    
    RET_IF_JSON_BAD(session_id_json, payload, "id", json_type_int);
    session_id = json_object_get_uint64(session_id_json);

    session = server_get_user_session(th->server, session_id);
    if (!session)
        return "Invalid session ID or session expired";

    user = server_db_get_user_from_id(&th->db, session->user_id);
    if (!user)
    {
        server_del_user_session(th->server, session);
        return "Could not find user from session";
    }

    return server_set_client_logged_in(th, client, user, session, respond_json);
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
server_create_user_session(server_thread_t* th, client_t* client, 
                             const char* username, json_object* respond_json)
{
    session_t* session;

    dbuser_t* user = server_db_get_user_from_name(&th->db, username);
    if (!user)
        return "Invalid username";

    session = server_new_user_session(th->server, client);
    session->user_id = user->user_id;

    return server_set_client_logged_in(th, client, user, session, respond_json);
}

const char* 
server_handle_not_logged_in_client(server_thread_t* th, 
                                   client_t* client, 
                                   json_object* payload, 
                                   json_object* respond_json, 
                                   const char* cmd)
{
    json_object* username_json;
    json_object* password_json;
    json_object* displayname_json;
    const char* username;
    const char* password;
    const char* displayname;
    const char* errmsg = NULL;

    if (!strcmp(cmd, "session"))
        return server_handle_user_session(th, client, payload, 
                                            respond_json);

    RET_IF_JSON_BAD(username_json, payload, "username", json_type_string);
    RET_IF_JSON_BAD(password_json, payload, "password", json_type_string);

    username = json_object_get_string(username_json);
    password = json_object_get_string(password_json);

    if (!strcmp(cmd, "login"))
        errmsg = server_handle_client_login(th, username, password);
    else if (!strcmp(cmd, "register"))
    {
        RET_IF_JSON_BAD(displayname_json, payload, "displayname", json_type_string);
        displayname = json_object_get_string(displayname_json);
        errmsg = server_handle_client_register(th, username, 
                                               displayname, password);
    }
    else
        return "Not logged in.";

    if (!errmsg)
        errmsg = server_create_user_session(th, client, username, respond_json);
    return errmsg;
}
