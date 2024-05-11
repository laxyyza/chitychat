#ifndef _SERVER_USER_LOGIN_H_
#define _SERVER_USER_LOGIN_H_

#include "chat/db_def.h"

typedef struct 
{
    char password[DB_PASSWORD_MAX];
    bool do_session;
} user_login_param_t;

const char* 
server_client_register(eworker_t* th, 
                              client_t* client, 
                              json_object* payload,
                              json_object* resp);

const char* 
server_client_login(eworker_t* th, 
                    client_t* client, 
                    json_object* payload, 
                    json_object* respond_json);

const char* 
server_client_login_session(eworker_t* th, 
                            client_t* client, 
                            json_object* payload, 
                            json_object* respond_json);

#endif // _SERVER_USER_LOGIN_H_
