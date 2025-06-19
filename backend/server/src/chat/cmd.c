#include "chat/cmd.h"
#include "chat/user.h"
#include "server_ht.h"
#include "server.h"

#define CMD_HT_SIZE 30

bool 
server_init_chatcmd_logged_in(server_t* server)
{
    if (!server_new_chatcmd(server, "client_user_info",
                            server_client_user_info, 
                            CHATCMD_PERM_LOGGED_IN))
        return false;

    if (!server_new_chatcmd(server, "get_user",
                            server_get_user,
                            CHATCMD_PERM_LOGGED_IN))
        return false;

    if (!server_new_chatcmd(server, "edit_account",
                            server_user_edit_account,
                            CHATCMD_PERM_LOGGED_IN))
        return false;

    return true;
}

// bool 
// server_init_chatcmd_not_logged_in(server_t* server)
// {
//     if (!server_new_chatcmd(server, "register",
//                             server_client_register,
//                             CHATCMD_PERM_NONE)) 
//         return false;
//
//     if (!server_new_chatcmd(server, "login",
//                             server_client_login,
//                             CHATCMD_PERM_NONE)) 
//         return false;
//
//     // if (!server_new_chatcmd(server, "session",
//     //                         server_client_login_session,
//     //                         CHATCMD_PERM_NONE)) 
//     //     return false;
//
//     return true;
// }

bool    
server_init_chatcmd(server_t* server)
{
    if (server_ght_init(&server->chat_cmd_ht, CMD_HT_SIZE, 
                        free /* libc free() */) == false) 
        return false;

    if (server_init_chatcmd_logged_in(server) == false)
        return false;

    // if (server_init_chatcmd_not_logged_in(server) == false)
    //     return false;

    return true;
}

bool    
server_new_chatcmd(server_t* server, 
                   const char* cmd, 
                   chatcmd_callback_t callback,
                   i32 perms)
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

    strncpy(chatcmd->cmd, cmd, CMD_STR_MAX - 1);
    chatcmd->cmd_hash = server_ght_hashstr(chatcmd->cmd);
    chatcmd->callback = callback;
    chatcmd->perms = perms;

    return server_ght_insert(ht, chatcmd->cmd_hash, chatcmd);
}

static void 
server_publish_cmd(server_t* server, client_t* client, const char* cmd, json_object* payload)
{
    u64 len;
    const char* str;
    char subject[SUBJECT_LEN];
    json_object* json = json_object_new_object();
    json_object_object_add(json, "user_id", json_object_new_uint64(client->dbuser->user_id));
    json_object_object_add(json, "payload", json_object_get(payload));

    snprintf(subject, SUBJECT_LEN - 1, "ws.cmd.%s", cmd);

    str = json_object_to_json_string_length(json, JSON_C_TO_STRING_NOSLASHESCAPE, &len);
    natsConnection_PublishRequest(server->nats.conn, subject, server->nats.subj_ws, str, len);

    json_object_put(json);
}

const char* 
server_exec_chatcmd(const char* cmd, 
                    eworker_t* ew, 
                    client_t* client, 
                    json_object* payload, 
                    json_object* resp)
{
    u64 hash_key;
    const char* ret;
    server_chatcmd_t* chatcmd;
    server_ght_t* ht = &ew->server->chat_cmd_ht;

    hash_key = server_ght_hashstr(cmd);

    chatcmd = server_ght_get(ht, hash_key);
    if (chatcmd == NULL)
    {
        if (client->state & CLIENT_STATE_LOGGED_IN)
        {
            server_publish_cmd(ew->server, client, cmd, payload);
            return NULL;
        }
        else
            return "Command not found";
    }
    else if (!(chatcmd->perms & client->state))
    {
        info("not perm: [%s]:%s\n", client->addr.ip_str, client->addr.serv);
        return "Require permission";
    }

    verbose("%s executing '%s' (hash: %zu)...\n", 
            client->addr.ip_str, chatcmd->cmd, chatcmd->cmd_hash);

    ret = chatcmd->callback(ew, client, payload, resp);

    return ret;
}
