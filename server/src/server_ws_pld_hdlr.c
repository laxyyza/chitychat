#include "server_ws_pld_hdlr.h"

static void 
server_add_user_in_json(dbuser_t* dbuser, json_object* json)
{
    json_object_object_add(json, "id", 
            json_object_new_int(dbuser->user_id));
    json_object_object_add(json, "username", 
            json_object_new_string(dbuser->username));
    json_object_object_add(json, "displayname", 
            json_object_new_string(dbuser->displayname));
    json_object_object_add(json, "bio", 
            json_object_new_string(dbuser->bio));
    json_object_object_add(json, "created_at", 
            json_object_new_string(dbuser->created_at));
    json_object_object_add(json, "pfp_name", 
            json_object_new_string(dbuser->pfp_hash));
}

static const char* 
client_user_info(client_t* client, json_object* respond_json)
{
    json_object_object_add(respond_json, "type", 
                json_object_new_string("client_user_info"));
    server_add_user_in_json(client->dbuser, respond_json);

    ws_json_send(client, respond_json);

    return NULL;
}

static const char* 
group_create(server_t* server, client_t* client, json_object* payload, 
        json_object* respond_json)
{
    json_object* name_json = json_object_object_get(payload, "name");
    const char* name = json_object_get_string(name_json);

    dbgroup_t new_group;
    memset(&new_group, 0, sizeof(dbgroup_t));
    new_group.owner_id = client->dbuser->user_id;
    strncpy(new_group.displayname, name, DB_DISPLAYNAME_MAX);

    if (!server_db_insert_group(server, &new_group))
    {
        return "Failed to create group";
    }

    info("Creating new group, id: %u, name: '%s', owner_id: %u\n", 
                new_group.group_id, name, new_group.owner_id);

    json_object_object_add(respond_json, "type", 
            json_object_new_string("client_groups"));
    json_object_object_add(respond_json, "groups", 
            json_object_new_array_ext(1));
    json_object* group_array_json = json_object_object_get(respond_json, 
                "groups");

    dbgroup_t* g = server_db_get_group(server, new_group.group_id);
    json_object* group_json = json_object_new_object();
    json_object_object_add(group_json, "id", 
            json_object_new_int(g->group_id));
    json_object_object_add(group_json, "name", 
            json_object_new_string(g->displayname));
    json_object_object_add(group_json, "desc", 
            json_object_new_string(g->desc));

    u32 n_members;
    dbuser_t* gmembers = server_db_get_group_members(server, 
                g->group_id, &n_members);
    
    json_object_object_add(group_json, "members_id",    
            json_object_new_array_ext(n_members));
    json_object* members_array_json = json_object_object_get(group_json, 
            "members_id");

    // TODO: Add full user info in packet
    for (u32 m = 0; m < n_members; m++)
    {
        const dbuser_t* member = gmembers + m;
        json_object_array_add(members_array_json, 
                json_object_new_int(member->user_id));
    }

    json_object_array_add(group_array_json, group_json);

    ws_json_send(client, respond_json);
    
    free(g);
    free(gmembers);

    return NULL;
}

static const char* 
client_groups(server_t* server, client_t* client, json_object* respond_json) 
{
    dbgroup_t* groups;
    u32 n_groups;

    groups = server_db_get_user_groups(server, client->dbuser->user_id, 
            &n_groups);

    if (!groups)
    {
        return "Server failed to get user groups";
    }

    json_object_object_add(respond_json, "type", 
            json_object_new_string("client_groups"));
    json_object_object_add(respond_json, "groups", 
                json_object_new_array_ext(n_groups));
    json_object* group_array_json = json_object_object_get(respond_json, 
                "groups");

    for (size_t i = 0; i < n_groups; i++)
    {
        const dbgroup_t* g = groups + i;
        json_object* group_json = json_object_new_object();
        json_object_object_add(group_json, "id", 
                json_object_new_int(g->group_id));
        json_object_object_add(group_json, "name", 
                json_object_new_string(g->displayname));
        json_object_object_add(group_json, "desc", 
                json_object_new_string(g->desc));

        u32 n_members;
        dbuser_t* gmembers = server_db_get_group_members(server, 
                g->group_id, &n_members);
        
        json_object_object_add(group_json, "members_id", 
                json_object_new_array_ext(n_members));
        json_object* members_array_json = json_object_object_get(group_json, 
                "members_id");

        // TODO: Add full user info in packet
        for (u32 m = 0; m < n_members; m++)
        {
            const dbuser_t* member = gmembers + m;
            json_object_array_add(members_array_json, 
                    json_object_new_int(member->user_id));
        }

        json_object_array_add(group_array_json, group_json);
        free(gmembers);
    }

    ws_json_send(client, respond_json);

    free(groups);

    return NULL;
}

static const char* 
get_user(server_t* server, client_t* client, json_object* payload, 
        json_object* respond_json)
{
    json_object* user_id_json = json_object_object_get(payload, "id");
    u64 user_id = json_object_get_int(user_id_json);

    dbuser_t* dbuser = server_db_get_user_from_id(server, user_id);
    if (!dbuser)
        return "User not found";

    json_object_object_add(respond_json, "type", 
            json_object_new_string("get_user"));
    server_add_user_in_json(dbuser, respond_json);

    ws_json_send(client, respond_json);

    free(dbuser);

    return NULL;
}

static const char* 
get_all_groups(server_t* server, client_t* client, json_object* respond_json)
{
    u32 n_groups;
    dbgroup_t* groups = server_db_get_all_groups(server, &n_groups);
    if (!groups)
        return "Failed to get groups";

    json_object_object_add(respond_json, "type", 
            json_object_new_string("get_all_groups"));
    json_object_object_add(respond_json, "groups", 
            json_object_new_array_ext(n_groups));
    json_object* groups_json = json_object_object_get(respond_json, 
            "groups");

    for (size_t i = 0; i < n_groups; i++)
    {
        dbgroup_t* g = groups + i;

        json_object* group_json = json_object_new_object();
        json_object_object_add(group_json, "id", 
                json_object_new_int(g->group_id));
        json_object_object_add(group_json, "owner_id", 
                json_object_new_int(g->owner_id));
        json_object_object_add(group_json, "name", 
                json_object_new_string(g->displayname));
        json_object_object_add(group_json, "desc", 
                json_object_new_string(g->desc));
        json_object_array_add(groups_json, group_json);
    }

    ws_json_send(client, respond_json);

    free(groups);

    return NULL;
}

static const char* 
join_group(server_t* server, client_t* client, json_object* payload, 
        json_object* respond_json)
{
    json_object* group_id_json = json_object_object_get(payload, 
            "group_id");
    const u64 group_id = json_object_get_int(group_id_json);

    dbgroup_t* group = server_db_get_group(server, group_id);
    if (!group)
        return "Group not found";

    if (!server_db_insert_group_member(server, group->group_id, 
            client->dbuser->user_id))
        return "Failed to join";

    json_object_object_add(respond_json, "type", 
            json_object_new_string("client_groups"));
    json_object_object_add(respond_json, "groups", 
            json_object_new_array_ext(1));
    json_object* array_json = json_object_object_get(respond_json, 
            "groups");

    // TODO: DRY!
    json_object* group_json = json_object_new_object();
    json_object_object_add(group_json, "id", 
            json_object_new_int(group->group_id));
    json_object_object_add(group_json, "name", 
            json_object_new_string(group->displayname));
    json_object_object_add(group_json, "desc", 
            json_object_new_string(group->desc));

    u32 n_members;
    dbuser_t* gmembers = server_db_get_group_members(server, 
            group->group_id, &n_members);

    json_object_object_add(group_json, "members_id", 
            json_object_new_array_ext(n_members));
    json_object* members_array_json = json_object_object_get(group_json, 
            "members_id");

    json_object* other_clients_respond = json_object_new_object();
    json_object_object_add(other_clients_respond, "type", 
        json_object_new_string("join_group"));
    json_object_object_add(other_clients_respond, "user_id", 
        json_object_new_int(client->dbuser->user_id));
    json_object_object_add(other_clients_respond, "group_id", 
        json_object_new_int(group_id));

    for (u32 m = 0; m < n_members; m++)
    {
        const dbuser_t* member = gmembers + m;
        const client_t* member_client = server_get_client_user_id(server, 
                member->user_id);

        if (member_client && member_client != client)
        {
            ws_json_send(member_client, other_clients_respond);
        }

        json_object_array_add(members_array_json, 
                json_object_new_int(member->user_id));
    }

    json_object_put(other_clients_respond);

    json_object_array_add(array_json, group_json);

    // TODO: Send to all other online clients in that group that a new user joined
    
    ws_json_send(client, respond_json);

    free(gmembers);
    free(group);

    return NULL;
}

static const char* 
group_msg(server_t* server, client_t* client, json_object* payload, 
        json_object* respond_json)
{
    json_object* group_id_json = json_object_object_get(payload, 
            "group_id");
    json_object* content_json = json_object_object_get(payload, 
            "content");
    const u64 group_id = json_object_get_int(group_id_json);
    const char* content = json_object_get_string(content_json);
    const u64 user_id = client->dbuser->user_id;

    dbmsg_t new_msg = {
        .user_id = user_id,
        .group_id = group_id,
    };
    strncpy(new_msg.content, content, DB_MESSAGE_MAX);

    if (!server_db_insert_msg(server, &new_msg))
    {
        return "Failed to send message";
    }

    dbmsg_t* dbmsg = server_db_get_msg(server, new_msg.msg_id);
    // Update all other online group members

    u32 n_memebrs;
    dbuser_t* gmemebrs = server_db_get_group_members(server, group_id, 
                                                    &n_memebrs);
    if (!gmemebrs)
    {
        free(dbmsg);
        return "Failed to get group members";
    }

    json_object_object_add(respond_json, "type", 
            json_object_new_string("group_msg"));
    json_object_object_add(respond_json, "msg_id", 
            json_object_new_int(dbmsg->msg_id));
    json_object_object_add(respond_json, "group_id", 
            json_object_new_int(dbmsg->group_id));
    json_object_object_add(respond_json, "user_id", 
            json_object_new_int(dbmsg->user_id));
    json_object_object_add(respond_json, "content", 
            json_object_new_string(dbmsg->content));
    json_object_object_add(respond_json, "timestamp", 
            json_object_new_string(dbmsg->timestamp));

    for (size_t i = 0; i < n_memebrs; i++)
    {
        const dbuser_t* member = gmemebrs + i;
        const client_t* member_client = server_get_client_user_id(server, 
                member->user_id);

        // If member_client is not NULL it means they are online.
        if (member_client)
            ws_json_send(member_client, respond_json);
    }

    free(dbmsg);
    free(gmemebrs);

    return NULL;
}

static const char* 
get_group_msgs(server_t* server, client_t* client, json_object* payload, 
        json_object* respond_json)
{
    json_object* limit_json;
    json_object* group_id_json;
    json_object* offset_json;
    u64 group_id;
    u32 limit;
    u32 offset;
    u32 n_msgs;
    dbmsg_t* msgs;

    group_id_json = json_object_object_get(payload, 
            "group_id");
    limit_json = json_object_object_get(payload, 
            "limit");
    offset_json = json_object_object_get(payload, "offset");

    group_id = json_object_get_int(group_id_json);
    limit = (limit_json) ? json_object_get_int(limit_json) : -1;
    offset = (offset_json) ? json_object_get_int(offset_json) : 0;

    msgs = server_db_get_msgs_from_group(server, group_id, limit, 
            offset, &n_msgs);

    json_object_object_add(respond_json, "type", 
            json_object_new_string("get_group_msgs"));
    json_object_object_add(respond_json, "group_id", 
            json_object_new_int(group_id));
    json_object_object_add(respond_json, "messages", 
            json_object_new_array_ext(n_msgs));
    json_object* msgs_json = json_object_object_get(respond_json, "messages");

    for (u32 i = 0; i < n_msgs; i++)
    {
        const dbmsg_t* msg = msgs + i;

        json_object* msg_in_array = json_object_new_object();
        json_object_object_add(msg_in_array, "msg_id", 
                json_object_new_int(msg->msg_id));
        json_object_object_add(msg_in_array, "user_id", 
                json_object_new_int(msg->user_id));
        json_object_object_add(msg_in_array, "group_id", 
                json_object_new_int(msg->group_id));
        json_object_object_add(msg_in_array, "content", 
                json_object_new_string(msg->content));
        json_object_object_add(msg_in_array, "timestamp", 
                json_object_new_string(msg->timestamp));
        json_object_array_add(msgs_json, msg_in_array);
    }

    ws_json_send(client, respond_json);

    free(msgs);
    return NULL;
}

static const char* 
edit_account(server_t* server, client_t* client, json_object* payload, 
        json_object* respond_json)
{
    json_object* new_username_json = json_object_object_get(payload, 
            "new_username");
    json_object* new_displayname_json = json_object_object_get(payload, 
            "new_displayname");
    json_object* new_pfp_json = json_object_object_get(payload, 
            "new_pfp");

    const char* new_username = json_object_get_string(new_username_json);
    const char* new_displayname = json_object_get_string(new_displayname_json);
    const bool new_pfp = json_object_get_boolean(new_pfp_json);

    if (!server_db_update_user(server, new_username, new_displayname, 
            NULL, client->dbuser->user_id))
    {
        return "Failed to update user"; 
    }

    if (new_pfp)
    {
        upload_token_t* upload_token = server_new_upload_token(server, 
                client->dbuser->user_id);
        // Create new upload token

        if (upload_token == NULL)
            return "Failed to create upload token";

        json_object_object_add(respond_json, "type", 
                json_object_new_string("edit_account"));
        json_object_object_add(respond_json, "upload_token", 
                json_object_new_uint64(upload_token->token));

        ws_json_send(client, respond_json);
    }

    debug("New username: %s, new display name: %s, new pfp: %d\n", 
            new_username, new_displayname, new_pfp);

    return NULL;
}

static const char* 
server_handle_logged_in_client(server_t* server, client_t* client, 
        json_object* payload, json_object* respond_json, const char* type)
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
        return get_all_groups(server, client, respond_json);
    else if (!strcmp(type, "join_group"))
        return join_group(server, client, payload, respond_json);
    else if (!strcmp(type, "group_msg"))
        return group_msg(server, client, payload, respond_json);
    else if (!strcmp(type, "get_group_msgs"))
        return get_group_msgs(server, client, payload, respond_json);
    else if (!strcmp(type, "edit_account"))
        return edit_account(server, client, payload, respond_json);
    else
        warn("Unknown packet type: '%s'\n", type);

    return NULL;
}

static const char* 
server_set_client_logged_in(server_t* server, client_t* client, 
                session_t* session, json_object* respond_json)
{
    dbuser_t* dbuser;

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
server_handle_client_login(server_t* server, client_t* client, 
                const char* username, const char* password)
{
    dbuser_t* user = server_db_get_user_from_name(server, username);
    if (!user)
        goto error;

    u8 hash_login[SERVER_HASH_SIZE];
    u8* salt = user->salt;
    server_hash(password, salt, hash_login);

    if (memcmp(user->hash, hash_login, SERVER_HASH_SIZE) != 0)
        goto error;

    debug("User:%u, %s '%s' logged in.\n", 
            user->user_id, user->username, user->displayname);

    free(user);

    return NULL;
error:
    return "Incorrect Username or Password";
}

static const char* 
server_handle_client_register(server_t* server, client_t* client, 
            const char* username, json_object* displayname_json, 
            const char* displayname, const char* password)
{
    if (!displayname_json || !displayname)
        return "Require display name.";

    dbuser_t new_user;
    memset(&new_user, 0, sizeof(dbuser_t));
    strncpy(new_user.username, username, DB_USERNAME_MAX);
    strncpy(new_user.displayname, displayname, DB_USERNAME_MAX);

    getrandom(new_user.salt, SERVER_SALT_SIZE, 0);
    server_hash(password, new_user.salt, new_user.hash);

    if (!server_db_insert_user(server, &new_user))
        return "Username already taken";

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

static const char* 
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
        errmsg = server_handle_client_register(server, client, username, 
                displayname_json, displayname, password);
    else
        return "Not logged in.";

    if (!errmsg)
        server_create_client_session(server, client, username, respond_json);
    return errmsg;
}

enum client_recv_status 
server_ws_handle_text_frame(server_t* server, client_t* client, 
                            char* buf, size_t buf_len) 
{
    char* buf_print = strndup(buf, buf_len);
    verbose("Web Socket message from fd:%d, IP: %s:%s:\n\t'%s'\n",
         client->addr.sock, client->addr.ip_str, client->addr.serv, buf_print);
    free(buf_print);

    json_object* respond_json = json_object_new_object();
    json_tokener* tokener = json_tokener_new();
    json_object* payload = json_tokener_parse_ex(tokener, buf, 
                                            buf_len);
    json_tokener_free(tokener);

    if (!payload)
    {
        warn("WS JSON parse failed, message:\n%s\n", buf);
        server_del_client(server, client);
        return RECV_DISCONNECT;
    }

    json_object* type_json = json_object_object_get(payload, "type");
    const char* type = json_object_get_string(type_json);
    const char* error_msg = NULL;

    if (client->state & CLIENT_STATE_LOGGED_IN)
        error_msg = server_handle_logged_in_client(server, client, payload, 
                                                    respond_json, type);
    else
        error_msg = server_handle_not_logged_in_client(server, client, 
                                            payload, respond_json, type);

    if (error_msg)
    {
        verbose("Sending error: %s\n", error_msg);
        json_object_object_add(respond_json, "type", 
                                json_object_new_string("error"));
        json_object_object_add(respond_json, "error_msg", 
                                    json_object_new_string(error_msg));

        ws_json_send(client, respond_json);
    }

    json_object_put(payload);
    json_object_put(respond_json);

    return RECV_OK;
}