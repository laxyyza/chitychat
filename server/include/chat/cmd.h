#ifndef _SERVER_CHAT_CMD_H_
#define _SERVER_CHAT_CMD_H_

#include "chat/user_session.h"
#include "common.h"
#include "server_tm.h"
#include "server_client.h"

#define CMD_STR_MAX 32

typedef const char* (*chatcmd_callback_t)(server_thread_t* th, client_t* client, 
                                          json_object* payload, json_object* resp);

typedef struct 
{
    u64                 cmd_hash;
    char                cmd[CMD_STR_MAX];
    chatcmd_callback_t  callback;
} server_chatcmd_t;

bool    server_init_chatcmd(server_t* server);
bool    server_new_chatcmd(server_t* server, const char* cmd, chatcmd_callback_t callback);
const char* server_exec_chatcmd(const char* cmd, 
                                server_thread_t* th, 
                                client_t* client, 
                                json_object* payload, 
                                json_object* resp);

#endif // _SERVER_CHAT_CMD_H_
