#include "server_user_req.h"
#include "server.h"
#include "server_user_group.h"
#include "server_user_account.h"

const char* 
server_handle_logged_in_client(server_thread_t* th, client_t* client, 
        json_object* payload, json_object* respond_json, const char* type)
{
    if (!strcmp(type, "client_user_info"))
        return server_client_user_info(client, respond_json);
    else if (!strcmp(type, "group_create"))
        return server_group_create(th, client, payload, respond_json);
    else if (!strcmp(type, "client_groups"))
        return server_client_groups(th, client, respond_json);
    else if (!strcmp(type, "get_user"))
        return server_get_user(th, client, payload, respond_json);
    else if (!strcmp(type, "get_all_groups"))
        return server_get_all_groups(th, client, respond_json);
    else if (!strcmp(type, "join_group"))
        return server_join_group(th, client, payload, respond_json);
    else if (!strcmp(type, "group_msg"))
        return server_group_msg(th, client, payload, respond_json);
    else if (!strcmp(type, "get_group_msgs"))
        return get_group_msgs(th, client, payload, respond_json);
    else if (!strcmp(type, "edit_account"))
        return server_user_edit_account(th, client, payload, respond_json);
    else if (!strcmp(type, "create_group_code"))
        return server_create_group_code(th, client, payload, respond_json);
    else if (!strcmp(type, "join_group_code"))
        return server_join_group_code(th, client, payload, respond_json);
    else if (!strcmp(type, "get_group_codes"))
        return server_get_group_codes(th, client, payload, respond_json);
    else
        warn("Unknown packet type: '%s'\n", type);

    return NULL;
}
