#ifndef _SERVER_USER_REQ_H_
#define _SERVER_USER_REQ_H_

#include "server_websocket.h"

const char* server_handle_logged_in_client(server_thread_t* th, 
                client_t* client, json_object* payload, 
                json_object* respond_json, const char* type);

#endif // _SERVER_USER_REQ_H_
