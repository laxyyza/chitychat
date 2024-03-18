#include "server_user_req.h"
#include "server.h"
#include "server_group.h"

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

const char* 
server_handle_logged_in_client(server_t* server, client_t* client, 
        json_object* payload, json_object* respond_json, const char* type)
{
    if (!strcmp(type, "client_user_info"))
        return client_user_info(client, respond_json);
    else if (!strcmp(type, "group_create"))
        return server_group_create(server, client, payload, respond_json);
    else if (!strcmp(type, "client_groups"))
        return server_client_groups(server, client, respond_json);
    else if (!strcmp(type, "get_user"))
        return get_user(server, client, payload, respond_json);
    else if (!strcmp(type, "get_all_groups"))
        return server_get_all_groups(server, client, respond_json);
    else if (!strcmp(type, "join_group"))
        return server_join_group(server, client, payload, respond_json);
    else if (!strcmp(type, "group_msg"))
        return server_group_msg(server, client, payload, respond_json);
    else if (!strcmp(type, "get_group_msgs"))
        return get_group_msgs(server, client, payload, respond_json);
    else if (!strcmp(type, "edit_account"))
        return edit_account(server, client, payload, respond_json);
    else
        warn("Unknown packet type: '%s'\n", type);

    return NULL;
}

