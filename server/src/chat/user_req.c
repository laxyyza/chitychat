#include "chat/user_req.h"
#include "chat/cmd.h"

const char* 
server_handle_logged_in_client(server_thread_t* th, 
                               client_t* client, 
                               json_object* payload, 
                               json_object* respond_json, 
                               const char* cmd)
{
    const char* ret;

    ret = server_exec_chatcmd(cmd, th, client, payload, respond_json);

    return ret;

    // // Get Client User info
    // if (!strcmp(cmd, "client_user_info"))
    //     return server_client_user_info(th, client, NULL, respond_json);
    //
    // // Create Group
    // else if (!strcmp(cmd, "group_create"))
    //     return server_group_create(th, client, payload, respond_json);
    //
    // // Get Client Groups
    // else if (!strcmp(cmd, "client_groups"))
    //     return server_client_groups(th, client, respond_json);
    //
    // // Get user info
    // else if (!strcmp(cmd, "get_user"))
    //     return server_get_user(th, client, payload, respond_json);
    //
    // // Get all public groups
    // else if (!strcmp(cmd, "get_all_groups"))
    //     return server_get_all_groups(th, client, NULL, respond_json);
    //
    // // Join Group
    // else if (!strcmp(cmd, "join_group"))
    //     return server_join_group(th, client, payload, respond_json);
    //
    // // Send Group Message
    // else if (!strcmp(cmd, "group_msg"))
    //     return server_group_msg(th, client, payload, respond_json);
    //
    // // Get Messages in Group
    // else if (!strcmp(cmd, "get_group_msgs"))
    //     return server_get_group_msgs(th, client, payload, respond_json);
    //
    // // Edit User Account
    // else if (!strcmp(cmd, "edit_account"))
    //     return server_user_edit_account(th, client, payload, respond_json);
    //
    // // Create Group Invite Coded
    // else if (!strcmp(cmd, "create_group_code"))
    //     return server_create_group_code(th, client, payload, respond_json);
    //
    // // Join Group via invite code
    // else if (!strcmp(cmd, "join_group_code"))
    //     return server_join_group_code(th, client, payload, respond_json);
    //
    // // Get Group invite codes
    // else if (!strcmp(cmd, "get_group_codes"))
    //     return server_get_group_codes(th, client, payload, respond_json);
    //
    // // Delete Group Invite Code
    // else if (!strcmp(cmd, "delete_group_code"))
    //     return server_delete_group_code(th, client, payload, NULL);
    //   
    // // Delete Group Message
    // else if (!strcmp(cmd, "delete_msg"))
    //     return server_delete_group_msg(th, client, payload, respond_json);
    //
    // // Delete Group
    // else if (!strcmp(cmd, "delete_group"))
    //     return server_delete_group(th, client, payload, respond_json);
    /*
     * Future implementations:
     */
    /*
    else if (!strcmp(cmd, "edit_msg"))
        return server_edit_msg(th, client, payload, respond_json);

    else if (!strcmp(cmd, "edit_group"))
        return server_edit_group(th, client, payload, respond_json);

    else if (!strcmp(cmd, "delete_user_account"))
        return server_delete_user_account(th, client, payload, respond_json);
    */
    // Wrong command 
    // else
    //     return "Unknown command";
}
