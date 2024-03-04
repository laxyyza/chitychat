#include "server_ws_pld_hdlr.h"
#include <sys/random.h>

static session_t* server_find_session(server_t* server, u32 id)
{
    if (id == 0)
        return NULL;

    for (size_t i = 0; i < MAX_SESSIONS; i++)
        if (server->sessions[i].session_id == id)
            return &server->sessions[i];

    return NULL;
}

ssize_t ws_json_send(client_t* client, json_object* json)
{
    size_t len;
    const char* string = json_object_to_json_string_length(json, 0, &len);

    debug("Sending:\n\t%s\n", string);

    return ws_send(client, string, len);
}

static const char* client_user_info(client_t* client, json_object* respond_json)
{
    json_object_object_add(respond_json, "type", json_object_new_string("client_user_info"));
    json_object_object_add(respond_json, "id", json_object_new_int(client->dbuser.user_id));
    json_object_object_add(respond_json, "username", json_object_new_string(client->dbuser.username));
    json_object_object_add(respond_json, "displayname", json_object_new_string(client->dbuser.displayname));
    json_object_object_add(respond_json, "bio", json_object_new_string(client->dbuser.bio));

    ws_json_send(client, respond_json);

    return NULL;
}

static const char* group_create(server_t* server, client_t* client, json_object* payload, json_object* respond_json)
{
    json_object* name_json = json_object_object_get(payload, "name");
    const char* name = json_object_get_string(name_json);

    dbgroup_t new_group;
    memset(&new_group, 0, sizeof(dbgroup_t));
    new_group.owner_id = client->dbuser.user_id;
    strncpy(new_group.displayname, name, DB_DISPLAYNAME_MAX);

    if (!server_db_insert_group(server, &new_group))
    {
        return "Failed to create group";
    }

    // Not needed anymore.
    // server_db_insert_group_member(server, new_group.group_id, client->dbuser.user_id);

    info("Creating new group, id: %u, name: '%s', owner_id: %u\n", new_group.group_id, name, new_group.owner_id);

    json_object_object_add(respond_json, "type", json_object_new_string("group"));
    json_object_object_add(respond_json, "id", json_object_new_int(new_group.group_id));
    json_object_object_add(respond_json, "name", json_object_new_string(new_group.displayname));
    json_object_object_add(respond_json, "desc", json_object_new_string(new_group.desc));

    ws_json_send(client, respond_json);

    return NULL;
}

static const char* client_groups(server_t* server, client_t* client, json_object* respond_json)
{
    dbgroup_t* groups;
    u32 n_groups;

    groups = server_db_get_user_groups(server, client->dbuser.user_id, &n_groups);

    if (!groups)
    {
        return "Server failed to get user groups";
    }

    json_object_object_add(respond_json, "type", json_object_new_string("client_groups"));
    json_object_object_add(respond_json, "groups", json_object_new_array_ext(n_groups));
    json_object* group_array_json = json_object_object_get(respond_json, "groups");

    for (size_t i = 0; i < n_groups; i++)
    {
        const dbgroup_t* g = groups + i;
        json_object* group_json = json_object_new_object();
        json_object_object_add(group_json, "id", json_object_new_int(g->group_id));
        json_object_object_add(group_json, "name", json_object_new_string(g->displayname));
        json_object_object_add(group_json, "desc", json_object_new_string(g->desc));

        u32 n_members;
        dbgroup_member_t* gmembers = server_db_get_group_members(server, g->group_id, &n_members);
        
        json_object_object_add(group_json, "members_id", json_object_new_array_ext(n_members));
        json_object* members_array_json = json_object_object_get(group_json, "members_id");

        for (u32 m = 0; m < n_members; m++)
        {
            const dbgroup_member_t* member = gmembers + m;
            json_object_array_add(members_array_json, json_object_new_int(member->user_id));
        }

        json_object_array_add(group_array_json, group_json);
        free(gmembers);
    }

    ws_json_send(client, respond_json);

    free(groups);

    return NULL;
}

static const char* get_user(server_t* server, client_t* client, json_object* payload, json_object* respond_json)
{
    json_object* user_id_json = json_object_object_get(payload, "id");
    u64 user_id = json_object_get_int(user_id_json);

    dbuser_t* dbuser = server_db_get_user_from_id(server, user_id);
    if (!dbuser)
        return "User not found";

    json_object_object_add(respond_json, "type", json_object_new_string("get_user"));
    json_object_object_add(respond_json, "id", json_object_new_int(dbuser->user_id));
    json_object_object_add(respond_json, "username", json_object_new_string(dbuser->username));
    json_object_object_add(respond_json, "displayname", json_object_new_string(dbuser->displayname));
    json_object_object_add(respond_json, "bio", json_object_new_string(dbuser->bio));

    ws_json_send(client, respond_json);

    free(dbuser);

    return NULL;
}

static const char* get_all_groups(server_t* server, client_t* client, json_object* respond_json)
{
    u32 n_groups;
    dbgroup_t* groups = server_db_get_all_groups(server, &n_groups);
    if (!groups)
        return "Failed to get groups";

    json_object_object_add(respond_json, "type", json_object_new_string("get_all_groups"));
    json_object_object_add(respond_json, "groups", json_object_new_array_ext(n_groups));
    json_object* groups_json = json_object_object_get(respond_json, "groups");

    for (size_t i = 0; i < n_groups; i++)
    {
        dbgroup_t* g = groups + i;

        json_object* group_json = json_object_new_object();
        json_object_object_add(group_json, "id", json_object_new_int(g->group_id));
        json_object_object_add(group_json, "owner_id", json_object_new_int(g->owner_id));
        json_object_object_add(group_json, "name", json_object_new_string(g->displayname));
        json_object_object_add(group_json, "desc", json_object_new_string(g->desc));
        json_object_array_add(groups_json, group_json);
    }

    ws_json_send(client, respond_json);

    free(groups);

    return NULL;
}

static const char* join_group(server_t* server, client_t* client, json_object* payload, json_object* respond_json)
{
    json_object* group_id_json = json_object_object_get(payload, "group_id");
    const u64 group_id = json_object_get_int(group_id_json);

    dbgroup_t* group = server_db_get_group(server, group_id);
    if (!group)
        return "Group not found";

    if (!server_db_insert_group_member(server, group->group_id, client->dbuser.user_id))
        return "Failed to join";

    json_object_object_add(respond_json, "type", json_object_new_string("client_groups"));
    json_object_object_add(respond_json, "groups", json_object_new_array_ext(1));
    json_object* array_json = json_object_object_get(respond_json, "groups");

    // TODO: DRY!
    json_object* group_json = json_object_new_object();
    json_object_object_add(group_json, "id", json_object_new_int(group->group_id));
    json_object_object_add(group_json, "name", json_object_new_string(group->displayname));
    json_object_object_add(group_json, "desc", json_object_new_string(group->desc));
    json_object_array_add(array_json, group_json);

    // TODO: Send to all other online clients in that group that a new user joined
    
    ws_json_send(client, respond_json);

    free(group);

    return NULL;
}

static const char* server_handle_logged_in_client(server_t* server, client_t* client, json_object* payload, json_object* respond_json, const char* type)
{
    if (!strcmp(type, "client_user_info"))
        return client_user_info(client, respond_json);
    else if (!strcmp(type, "group_create"))
        return group_create(server, client, payload, respond_json);
    else if (!strcmp(type, "client_groups"))
        return client_groups(server, client, respond_json);
    else if (!strcmp(type, "get_user"))
        return get_user(server, client, payload, respond_json);
    else if (!strcmp(type, "get_all_groups"))
        get_all_groups(server, client, respond_json);
    else if (!strcmp(type, "join_group"))
        join_group(server, client, payload, respond_json);
    else
        warn("Unknown packet type: '%s'\n", type);

    return NULL;
}

static const char* server_handle_client_session(server_t* server, client_t* client, json_object* payload, json_object* respond_json)
{
    json_object* session_id_json = json_object_object_get(payload, "id");
    const u32 session_id = json_object_get_uint64(session_id_json);

    const session_t* session = server_find_session(server, session_id);
    if (!session)
    {
        return "Invalid session ID or session expired";
    }

    dbuser_t* dbuser = server_db_get_user_from_id(server, session->user_id);
    if (!dbuser)
    {
        return "Server cuold not find user in database";
    }
    memcpy(&client->dbuser, dbuser, sizeof(dbuser_t));
    free(dbuser);

    json_object_object_add(respond_json, "type", json_object_new_string("session"));
    json_object_object_add(respond_json, "id", json_object_new_uint64(session->session_id));

    size_t len;
    const char* respond = json_object_to_json_string_length(respond_json, 0, &len);

    if (ws_send(client, respond, len) != -1)
        client->state |= CLIENT_STATE_LOGGED_IN;

    return NULL;
}

static const char* server_handle_client_login(server_t* server, client_t* client, const char* username, const char* password)
{
    dbuser_t* user = server_db_get_user_from_name(server, username);
    if (!user)
        return "Incorrect Username or password";

    info("Found user: %zu|%s|%s|%s|%s\n", user->user_id, user->username, user->displayname, user->bio, user->password);

    if (strncmp(password, user->password, DB_PASSWORD_MAX))
        return "Incorrect Username or password";

    info("Logged in!!!\n");

    free(user);

    return NULL;
}

static const char* server_handle_client_register(server_t* server, client_t* client, const char* username, json_object* displayname_json, const char* displayname, const char* password)
{
    if (!displayname_json || !displayname)
        return "Require display name.";
    // Insert new user  

    dbuser_t new_user;
    memset(&new_user, 0, sizeof(dbuser_t));
    strncpy(new_user.username, username, DB_USERNAME_MAX);
    strncpy(new_user.displayname, displayname, DB_USERNAME_MAX);
    strncpy(new_user.password, password, DB_PASSWORD_MAX);

    if (!server_db_insert_user(server, &new_user))
        return "Username already taken";

    return NULL;
}

static void server_create_client_session(server_t* server, client_t* client, const char* username, json_object* respond_json)
{
    session_t* session;
    dbuser_t* user = server_db_get_user_from_name(server, username);
    if (!user)
    {
        warn("create_client_session: db_get_user_from_name returned NULL!\n");
        return;
    }

    for (size_t i = 0; i < MAX_SESSIONS; i++)
    {
        if (server->sessions[i].session_id == 0)
        {
            session = &server->sessions[i];
            break;
        }
    }

    getrandom(&session->session_id, sizeof(u32), 0);
    session->user_id = user->user_id;

    json_object_object_add(respond_json, "type", json_object_new_string("session"));
    json_object_object_add(respond_json, "id", json_object_new_uint64(session->session_id));

    size_t len;
    const char* respond = json_object_to_json_string_length(respond_json, 0, &len);

    info("New session created: %u\n", session->session_id);

    ws_send(client, respond, len);
    client->state |= CLIENT_STATE_LOGGED_IN;

    free(user);
}

static const char* server_handle_not_logged_in_client(server_t* server, client_t* client, json_object* payload, json_object* respond_json, const char* type)
{
    json_object* username_json = json_object_object_get(payload, "username");
    json_object* password_json = json_object_object_get(payload, "password");
    json_object* displayname_json = json_object_object_get(payload, "displayname");

    if (!strcmp(type, "session"))
        return server_handle_client_session(server, client, payload, respond_json);

    const char* username = json_object_get_string(username_json);
    const char* password = json_object_get_string(password_json);
    const char* displayname = json_object_get_string(displayname_json);

    if (!username_json || !username)
        return "Require username.";
    if (!password_json || !password)
        return "Require password.";

    const char* errmsg = NULL;

    if (!strcmp(type, "login"))
        errmsg = server_handle_client_login(server, client, username, password);
    else if (!strcmp(type, "register"))
        errmsg = server_handle_client_register(server, client, username, displayname_json, displayname, password);
    else
        return "Not logged in.";

    if (!errmsg)
        server_create_client_session(server, client, username, respond_json);
    return errmsg;
}

void server_ws_handle_text_frame(server_t* server, client_t* client, char* buf, size_t buf_len) 
{

    debug("Web Socket message from fd:%d, IP: %s:%s:\n\t'%s'\n",
         client->addr.sock, client->addr.ip_str, client->addr.serv, buf);

    json_object* respond_json = json_object_new_object();
    json_tokener* tokener = json_tokener_new();
    json_object* payload = json_tokener_parse_ex(tokener, buf, buf_len);
    json_tokener_free(tokener);
    json_object* type_json = json_object_object_get(payload, "type");
    const char* type = json_object_get_string(type_json);
    const char* error_msg = NULL;

    if (client->state & CLIENT_STATE_LOGGED_IN)
        error_msg = server_handle_logged_in_client(server, client, payload, respond_json, type);
    else
        error_msg = server_handle_not_logged_in_client(server, client, payload, respond_json, type);

    if (error_msg)
    {
        error("error:msg: %s\n", error_msg);
        json_object_object_add(respond_json, "type", json_object_new_string("error"));
        json_object_object_add(respond_json, "error_msg", json_object_new_string(error_msg));

        ws_json_send(client, respond_json);
    }

    json_object_put(payload);
    json_object_put(respond_json);
}