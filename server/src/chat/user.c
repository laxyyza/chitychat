#include "chat/user.h"
#include "chat/db_def.h"
#include "chat/rtusm.h"
#include "chat/ws_text_frame.h"
#include "chat/db_user.h"
#include "json_object.h"
#include "server_client.h"
#include "server_websocket.h"

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

static bool
verify_json_array_type(json_object* array_json, json_type type)
{
    size_t size = json_object_array_length(array_json);
    json_object* element;

    if (size == 0)
        return false;

    for (size_t i = 0; i < size; i++)
    {
        element = json_object_array_get_idx(array_json, i);
        if (!json_object_is_type(element, type))
            return false;
    }
    return true;
}

static const char*
do_get_users(UNUSED eworker_t* ew, dbcmd_ctx_t* ctx)
{
    if (ctx->ret == DB_ASYNC_ERROR)
        return "Failed to get users";

    json_object* resp = json_object_new_object();
    
    json_object_object_add(resp, "cmd", 
                           json_object_new_string("get_user"));
    json_object_object_add(resp, "users",
                           json_tokener_parse(ctx->param.str));

    ws_json_send(ctx->client, resp);

    json_object_put(resp);

    return NULL;
}

static dbuser_t**
get_rm_users_json(eworker_t* ew, json_object* array, size_t* n)
{
    dbuser_t** ret = NULL;
    client_t* client;
    size_t array_size = json_object_array_length(array);
    size_t pos = 0;
    json_object* int_json;
    u32 user_id;
    server_ght_t* uht = &ew->server->user_ht;

    for (ssize_t i = array_size - 1; i >= 0; i--)
    {
        int_json = json_object_array_get_idx(array, i);
        user_id = json_object_get_int(int_json);

        if ((client = server_ght_get(uht, user_id)))
        {
            if (ret == NULL)
                ret = calloc(array_size, sizeof(void*));
            ret[pos] = client->dbuser;
            pos++;
            json_object_array_del_idx(array, i, 1);
        }
    }
    *n = pos;
    return ret;
}

static const char*
get_users_from_db(eworker_t* ew, 
                  json_object* user_ids_array_json)
{
    const char* user_ids_array_str;
    size_t size;

    size = json_object_array_length(user_ids_array_json);
    if (size <= 0)
        return NULL;

    user_ids_array_str = json_object_to_json_string(user_ids_array_json);
    dbcmd_ctx_t ctx = {
        .exec = do_get_users
    };
    if (!db_async_get_user_array(&ew->db, user_ids_array_str, &ctx))
        return "Internal error: async-get-user-array";
    return NULL;
}

static void
send_users_from_clients(client_t* client, 
                       dbuser_t** users,
                       size_t n,
                       json_object* resp)
{
    json_object* users_array_json;
    json_object* user_json;
    dbuser_t* user;

    if (users == NULL)
        return;

    json_object_object_add(resp, "cmd",
                           json_object_new_string("get_user"));
    users_array_json = json_object_new_array_ext(n);

    for (size_t i = 0; i < n; i++)
    {
        user = users[i];
        user_json = json_object_new_object();
        server_add_user_in_json(user, user_json);
        json_object_array_put_idx(users_array_json, i, user_json);
    }
    json_object_object_add(resp, "users",
                           users_array_json);
    ws_json_send(client, resp);

    free(users);
}

const char* 
server_get_user(eworker_t* ew, 
                client_t* client, 
                json_object* payload, 
                json_object* resp)
{
    json_object* user_ids_array_json; 
    const char* errmsg;
    dbuser_t** online_users;
    size_t    n_online_users;

    RET_IF_JSON_BAD(user_ids_array_json, payload, "user_ids", json_type_array);
    if (verify_json_array_type(user_ids_array_json, json_type_int) == false)
        return "Invalid array";

    online_users = get_rm_users_json(ew, user_ids_array_json, &n_online_users);

    errmsg = get_users_from_db(ew, user_ids_array_json);
    send_users_from_clients(client, online_users, n_online_users, resp);

    // user_id = json_object_get_int(user_id_json);
    //
    // if ((connected_user = server_get_client_user_id(ew->server, user_id)))
    //     dbuser = connected_user->dbuser;
    // else
    // {
    //     dbuser = server_db_get_user_from_id(&ew->db, user_id);
    //     free_user = true;
    // }
    //
    // if (!dbuser)
    //     return "User not found";
    //
    // json_object_object_add(respond_json, "cmd", 
    //                        json_object_new_string("get_user"));
    // server_add_user_in_json(dbuser, respond_json);
    //
    // ws_json_send(client, respond_json);
    //
    // if (free_user)
    //     free(dbuser);

    return errmsg;
}

const char* 
server_client_user_info(eworker_t* ew, 
                        client_t* client, 
                        UNUSED json_object* payload,
                        json_object* respond_json)
{
    json_object_object_add(respond_json, "cmd", 
                           json_object_new_string("client_user_info"));
    server_add_user_in_json(client->dbuser, respond_json);

    ws_json_send(client, respond_json);

    server_rtusm_user_connect(ew, client->dbuser);

    return NULL;
}

const char* 
server_user_edit_account(eworker_t* ew, client_t* client, 
                         json_object* payload, UNUSED json_object* respond_json)
{
    // json_object* new_username_json;
    // json_object* new_displayname_json;
    json_object* new_pfp_json;
    const char* new_username = NULL;
    const char* new_displayname = NULL;
    bool new_pfp = false;

    // new_username_json = json_object_object_get(payload, "new_username");
    // new_displayname_json = json_object_object_get(payload, "new_displayname");
    new_pfp_json = json_object_object_get(payload, "new_pfp");

    // Check if username is type string
    // if (json_object_is_type(new_username_json, json_type_string))
    //     new_username = json_object_get_string(new_username_json);
    // else if (new_username_json != NULL)
    //     return "\"new_username\" is invalid";
    //
    // // Check if displayname is type string
    // if (json_object_is_type(new_displayname_json, json_type_string))
    //     new_displayname = json_object_get_string(new_displayname_json);
    // else if (new_displayname_json != NULL)
    //     return "\"new_displayname\" is invalid";
    //
    // Check if new_pfp is type boolean
    if (json_object_is_type(new_pfp_json, json_type_boolean))
        new_pfp = json_object_get_boolean(new_pfp_json);
    else if (new_pfp_json != NULL)
        return "\"new_pfp\" is invalid";
    //
    // if (!server_db_update_user(&ew->db, new_username, new_displayname, 
    //         NULL, client->dbuser->user_id))
    //     return "Failed to update user"; 

    if (new_pfp)
    {
        // Create new upload token
        upload_token_t* upload_token = server_new_upload_token(ew, 
                                                    client->dbuser->user_id);

        if (upload_token == NULL)
            return "Failed to create upload token";

        server_send_upload_token(client, "edit_account", upload_token);
    }

    debug("New username: %s, new display name: %s, new pfp: %d\n", 
            new_username, new_displayname, new_pfp);

    return NULL;
}
