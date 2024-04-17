#include "chat/user_account.h"
#include "chat/rtusm.h"
#include "chat/ws_text_frame.h"
#include "server_client.h"

static void 
server_add_user_in_json(dbuser_t* dbuser, json_object* json)
{
    json_object_object_add(json, "user_id", 
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
    json_object_object_add(json, "status", 
                           json_object_new_string(rtusm_get_status_str(dbuser->rtusm.status)));
}

const char* 
server_get_user(server_thread_t* th, client_t* client, 
                json_object* payload, json_object* respond_json)
{
    json_object* user_id_json; 
    dbuser_t* dbuser;
    u64 user_id; 
    const client_t* connected_user;
    bool free_user = false;

    RET_IF_JSON_BAD(user_id_json, payload, "user_id", json_type_int);
    user_id = json_object_get_int(user_id_json);

    if ((connected_user = server_get_client_user_id(th->server, user_id)))
        dbuser = connected_user->dbuser;
    else
    {
        dbuser = server_db_get_user_from_id(&th->db, user_id);
        free_user = true;
    }

    if (!dbuser)
        return "User not found";

    json_object_object_add(respond_json, "cmd", 
                           json_object_new_string("get_user"));
    server_add_user_in_json(dbuser, respond_json);

    ws_json_send(client, respond_json);

    if (free_user)
        free(dbuser);

    return NULL;
}

const char* 
server_client_user_info(server_thread_t* th, 
                        client_t* client, 
                        json_object* respond_json)
{
    json_object_object_add(respond_json, "cmd", 
                           json_object_new_string("client_user_info"));
    server_add_user_in_json(client->dbuser, respond_json);

    ws_json_send(client, respond_json);

    server_rtusm_user_connect(th, client->dbuser);

    return NULL;
}

const char* server_user_edit_account(server_thread_t* th, client_t* client, 
        json_object* payload, UNUSED json_object* respond_json)
{
    json_object* new_username_json;
    json_object* new_displayname_json;
    json_object* new_pfp_json;
    const char* new_username = NULL;
    const char* new_displayname = NULL;
    bool new_pfp = false;

    new_username_json = json_object_object_get(payload, "new_username");
    new_displayname_json = json_object_object_get(payload, "new_displayname");
    new_pfp_json = json_object_object_get(payload, "new_pfp");

    // Check if username is type string
    if (json_object_is_type(new_username_json, json_type_string))
        new_username = json_object_get_string(new_username_json);
    else if (new_username_json != NULL)
        return "\"new_username\" is invalid";

    // Check if displayname is type string
    if (json_object_is_type(new_displayname_json, json_type_string))
        new_displayname = json_object_get_string(new_displayname_json);
    else if (new_displayname_json != NULL)
        return "\"new_displayname\" is invalid";

    // Check if new_pfp is type boolean
    if (json_object_is_type(new_pfp_json, json_type_boolean))
        new_pfp = json_object_get_boolean(new_pfp_json);
    else if (new_pfp_json != NULL)
        return "\"new_pfp\" is invalid";

    if (!server_db_update_user(&th->db, new_username, new_displayname, 
            NULL, client->dbuser->user_id))
        return "Failed to update user"; 

    if (new_pfp)
    {
        // Create new upload token
        upload_token_t* upload_token = server_new_upload_token(th, 
                                                    client->dbuser->user_id);

        if (upload_token == NULL)
            return "Failed to create upload token";

        server_send_upload_token(client, "edit_account", upload_token);
    }

    debug("New username: %s, new display name: %s, new pfp: %d\n", 
            new_username, new_displayname, new_pfp);

    return NULL;
}
