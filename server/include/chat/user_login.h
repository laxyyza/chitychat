#ifndef _SERVER_USER_LOGIN_H_
#define _SERVER_USER_LOGIN_H_

#include "server_websocket.h"

const char* 
server_client_register(server_thread_t* th, 
                              client_t* client, 
                              json_object* payload,
                              json_object* resp);

const char* 
server_client_login(server_thread_t* th, 
                    client_t* client, 
                    json_object* payload, 
                    json_object* respond_json);

const char* 
server_client_login_session(server_thread_t* th, 
                            client_t* client, 
                            json_object* payload, 
                            json_object* respond_json);

#endif // _SERVER_USER_LOGIN_H_
