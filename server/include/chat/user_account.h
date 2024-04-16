#ifndef _SERVER_USER_ACCOUNT_H_
#define _SERVER_USER_ACCOUNT_H_

#include "server_websocket.h"

const char* server_client_user_info(client_t* client, 
                            json_object* respond_json);
const char* server_client_user_info(client_t* client, 
                            json_object* respond_json);
const char* server_get_user(server_thread_t* th, client_t* client, 
        json_object* payload, json_object* respond_json);
const char* server_user_edit_account(server_thread_t* th, client_t* client, 
        json_object* payload, json_object* respond_json);

#endif // _SERVER_USER_ACCOUNT_H_
