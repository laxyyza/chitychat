#include "server_websocket.h"
#include "server.h"
#include <sys/uio.h>
#include <json-c/json.h>
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

    return ws_send(client, string, len);
}

static const char* server_handle_logged_in_client(server_t* server, client_t* client, json_object* payload, json_object* respond_json, const char* type)
{
    if (!strcmp(type, "client_user_info"))
    {
        json_object_object_add(respond_json, "type", json_object_new_string("client_user_info"));
        json_object_object_add(respond_json, "id", json_object_new_int(client->dbuser.user_id));
        json_object_object_add(respond_json, "username", json_object_new_string(client->dbuser.username));
        json_object_object_add(respond_json, "displayname", json_object_new_string(client->dbuser.displayname));
        json_object_object_add(respond_json, "bio", json_object_new_string(client->dbuser.bio));

        ws_json_send(client, respond_json);
    }
    else if (!strcmp(type, "group_create"))
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
    }
    else if (!strcmp(type, "client_groups"))
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
        json_object* array_json = json_object_object_get(respond_json, "groups");

        for (size_t i = 0; i < n_groups; i++)
        {
            const dbgroup_t* g = groups + i;
            json_object* group_json = json_object_new_object();
            json_object_object_add(group_json, "id", json_object_new_int(g->group_id));
            json_object_object_add(group_json, "name", json_object_new_string(g->displayname));
            json_object_object_add(group_json, "desc", json_object_new_string(g->desc));
            json_object_array_add(array_json, group_json);
        }

        ws_json_send(client, respond_json);

        free(groups);
    }
    else if (!strcmp(type, "group_members"))
    {
        json_object* group_id_json = json_object_object_get(payload, "group_id");
        u64 group_id = json_object_get_int(group_id_json);

        u32 n_members;
        dbgroup_member_t* gmembers = server_db_get_group_members(server, group_id, &n_members);

        if (!n_members)
            return "Group has no members";

        json_object_object_add(respond_json, "type", json_object_new_string("group_members"));
        json_object_object_add(respond_json, "group_id", json_object_new_int(group_id));
        json_object_object_add(respond_json, "members", json_object_new_array_ext(n_members));
        json_object* members_array = json_object_object_get(respond_json, "members");

        for (size_t i = 0; i < n_members; i++)
        {
            const dbgroup_member_t* member = gmembers + i;
            json_object_array_add(members_array, json_object_new_int(member->user_id));
        }

        ws_json_send(client, respond_json);

        free(gmembers);
    }
    else if (!strcmp(type, "get_user"))
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
    }
    else if (!strcmp(type, "get_all_groups"))
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
    }
    else if (!strcmp(type, "join_group"))
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
    }
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

    if (!strcmp(type, "login"))
        server_handle_client_login(server, client, username, password);
    else if (!strcmp(type, "register"))
        server_handle_client_register(server, client, username, displayname_json, displayname, password);
    else
        return "Not logged in.";

    server_create_client_session(server, client, username, respond_json);
    return NULL;
}

static void server_ws_handle_text_frame(server_t *server, client_t *client,
                                        char *buf, size_t buf_len) {

    debug("Web Socket message from fd:%d, IP: %s:%s:\n\t'%s'\n",
         client->addr.sock, client->addr.ip_str, client->addr.serv, buf);

    // buf[buf_len] = 0x00;
    // size_t string_len = strlen(buf);

    // debug("Web Socket message from fd:%d, IP: %s:%s:\n\t'%s'\n",
    //      client->addr.sock, client->addr.ip_str, client->addr.serv, buf);

    // debug("string_len: %zu == %zu\n", string_len, buf_len);

    // print_each_char(buf);

    json_object* respond_json = json_object_new_object();
    json_tokener* tokener = json_tokener_new();
    json_object* payload = json_tokener_parse_ex(tokener, buf, buf_len);
    json_tokener_free(tokener);
    json_object* type_json = json_object_object_get(payload, "type");
    const char* type = json_object_get_string(type_json);
    const char* error_msg = NULL;

    // debug("payload: %p\n", payload);
    // debug("type_json: %p\n", type_json);
    // debug("type: '%s'\n", type);

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

void server_ws_parse(server_t* server, client_t* client, u8* buf, size_t buf_len)
{
    ws_t ws;
    memset(&ws, 0, sizeof(ws_t));
    size_t offset = sizeof(ws_frame_t);

    memcpy(&ws.frame, buf, sizeof(ws_frame_t));

    // debug("FIN: %u\n", ws.frame.fin);
    // debug("RSV1: %u\n", ws.frame.rsv1);
    // debug("RSV2: %u\n", ws.frame.rsv2);
    // debug("RSV3: %u\n", ws.frame.rsv3);
    // debug("MASK: %u\n", ws.frame.mask);
    // debug("OPCODE: %02X\n", ws.frame.opcode);
    // debug("PAYLOAD LEN: %u\n", ws.frame.payload_len);

    if (ws.frame.payload_len == 126)
    {
        ws.ext.u16 = WS_PAYLOAD_LEN16(buf);
        ws.payload_len = (u16)ws.ext.u16;
        offset += 2;
        debug("Payload len 16-bit: %zu\n", ws.payload_len);
    }
    else if (ws.frame.payload_len == 127)
    {
        ws.ext.u64 = WS_PAYLOAD_LEN64(buf);
        ws.payload_len = ws.ext.u64;
        offset += 8;
        debug("Payload len 64-bit\n");
    }
    else
        ws.payload_len   = ws.frame.payload_len;

    if (ws.frame.mask)
    {
        memcpy(ws.ext.maskkey, buf + offset, WS_MASKKEY_LEN);
        offset += WS_MASKKEY_LEN;
        ws.payload = (char*)&buf[offset];
        mask((u8*)ws.payload, ws.payload_len, ws.ext.maskkey, WS_MASKKEY_LEN);
    }
    else
        ws.payload = (char*)buf + offset;

    switch (ws.frame.opcode)
    {
        case WS_CONTINUE_FRAME:
            debug("CONTINUE FRAME: %s\n", ws.payload);
            break;
        case WS_TEXT_FRAME:
            server_ws_handle_text_frame(server, client, ws.payload, ws.payload_len);
            break;
        case WS_BINARY_FRAME:
            debug("BINARY FRAME: %s\n", ws.payload);
            break;
        case WS_CLOSE_FRAME:
        {
            debug("CLOSE FRAME: %s\n", ws.payload);
            server_del_client(server, client);
            break;
        }
        case WS_PING_FRAME:
            debug("PING FRAME: %s\n", ws.payload);
            break;
        case WS_PONG_FRAME:
            debug("PONG FRAME: %s\n", ws.payload);
            break;
        default:
            debug("UNKNOWN FRAME (%u): %s\n", ws.frame.opcode, ws.payload);
            break;
    }
}

ssize_t ws_send_adv(client_t *client, u8 opcode, const char *buf, size_t len,
                    const u8 *maskkey) {
    ssize_t bytes_sent = 0;
    struct iovec iov[4];
    size_t i = 0;
    ws_t ws = {
        .frame.fin = 1,
        .frame.rsv1 = 0,
        .frame.rsv2 = 0,
        .frame.rsv3 = 0,
        .frame.opcode = opcode,
        .frame.mask = (maskkey) ? 1 : 0,
        .frame.payload_len = len
    };

    iov[i].iov_base = &ws.frame;
    iov[i].iov_len = sizeof(ws_frame_t);

    if (len >= 125)
    {
        i++;
        if (len >= UINT16_MAX)
        {
            debug("WS SEND 64-bit\n");
            ws.frame.payload_len = 127;
            swpcpy((u8*)&ws.ext.u64, (u8*)&len, sizeof(u64));
            iov[i].iov_base = &ws.ext.u64;
            iov[i].iov_len = sizeof(u64);
        }
        else
        {
            debug("WS SEND 16-bit\n");
            ws.frame.payload_len = 126;
            swpcpy((u8*)&ws.ext.u16, (const u8*)&len, sizeof(u16));
            iov[i].iov_base = &ws.ext.u16;
            iov[i].iov_len = sizeof(u16);
        }
    }

    if (ws.frame.mask)
    {
        i++;
        iov[i].iov_base = (u8*)maskkey;
        iov[i].iov_len = WS_MASKKEY_LEN;

        if (buf)
            mask((u8*)buf, len, maskkey, WS_MASKKEY_LEN);

    }

    if (buf)
    {
        i++;
        iov[i].iov_base = (char*)buf;
        iov[i].iov_len = len;
    }

    i++;
    if ((bytes_sent = writev(client->addr.sock, iov, i)) == -1)
    {
        error("WS Send to (fd:%d, IP: %s:%s): %s\n",
            client->addr.sock, client->addr.ip_str, client->addr.serv
        );
    }

    return bytes_sent;
}

ssize_t ws_send(client_t* client, const char* buf, size_t len)
{
    return ws_send_adv(client, WS_TEXT_FRAME, buf, len, NULL);
}