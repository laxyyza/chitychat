#include "chat/cmd.h"
#include "chat/group.h"
#include "chat/user.h"
#include "server_ht.h"
#include "server.h"

#define CMD_HT_SIZE 30

bool    
server_init_chatcmd(server_t* server)
{
    if (server_ght_init(&server->chat_cmd_ht, CMD_HT_SIZE, 
                        free /* libc free() */) == false) 
        return false;

    if (!server_new_chatcmd(server, "client_user_info",
                            server_client_user_info))
        return false;

    if (!server_new_chatcmd(server, "client_groups",
                            server_client_groups))
        return false;

    if (!server_new_chatcmd(server, "group_create",
                            server_group_create))
        return false;

    if (!server_new_chatcmd(server, "get_user",
                            server_get_user))
        return false;

    if (!server_new_chatcmd(server, "get_all_groups",
                            server_get_all_groups))
        return false;

    if (!server_new_chatcmd(server, "join_group",
                            server_join_group))
        return false;

    if (!server_new_chatcmd(server, "group_msg",
                            server_group_msg))
        return false;

    if (!server_new_chatcmd(server, "get_group_msgs",
                            server_get_group_msgs))
        return false;

    if (!server_new_chatcmd(server, "edit_account",
                            server_user_edit_account))
        return false;

    if (!server_new_chatcmd(server, "create_group_code",
                            server_create_group_code))
        return false;

    if (!server_new_chatcmd(server, "join_group_code",
                            server_join_group_code))
        return false;

    if (!server_new_chatcmd(server, "get_group_codes",
                            server_get_group_codes))
        return false;

    if (!server_new_chatcmd(server, "delete_group_code",
                            server_delete_group_code))
        return false;

    if (!server_new_chatcmd(server, "delete_msg",
                            server_delete_group_msg))
        return false;

    if (!server_new_chatcmd(server, "delete_group",
                            server_delete_group))
        return false;

    return true;
}

bool    
server_new_chatcmd(server_t* server, const char* cmd, chatcmd_callback_t callback)
{
    server_chatcmd_t* chatcmd;
    server_ght_t* ht = &server->chat_cmd_ht;

    if (!server || !cmd || !callback)
        return false;

    chatcmd = calloc(1, sizeof(server_chatcmd_t));
    if (!chatcmd)
    {
        fatal("calloc() returned NULL!\n");
        return false;
    }

    strncpy(chatcmd->cmd, cmd, CMD_STR_MAX);
    chatcmd->cmd_hash = server_ght_hashstr(chatcmd->cmd);
    chatcmd->callback = callback;

    return server_ght_insert(ht, chatcmd->cmd_hash, chatcmd);
}

const char* 
server_exec_chatcmd(const char* cmd, 
                    server_thread_t* th, 
                    client_t* client, 
                    json_object* payload, 
                    json_object* resp)
{
    const char* ret;
    server_chatcmd_t* chatcmd;
    server_ght_t* ht = &th->server->chat_cmd_ht;

    chatcmd = server_ght_get(ht, server_ght_hashstr(cmd));
    if (chatcmd == NULL)
        return "Command not found.";

    verbose("Executing '%s' (hash: %zu)...\n", 
            chatcmd->cmd, chatcmd->cmd_hash);

    ret = chatcmd->callback(th, client, payload, resp);

    return ret;
}
