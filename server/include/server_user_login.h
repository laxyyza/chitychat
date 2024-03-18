#ifndef _SERVER_USER_LOGIN_H_
#define _SERVER_USER_LOGIN_H_

#include "server_websocket.h"

const char* server_handle_not_logged_in_client(server_t* server, client_t* client, 
                json_object* payload, json_object* respond_json, const char* type);

#endif // _SERVER_USER_LOGIN_H_