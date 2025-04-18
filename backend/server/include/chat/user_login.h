#ifndef _SERVER_USER_LOGIN_H_
#define _SERVER_USER_LOGIN_H_

#include "chat/db_def.h"

typedef struct 
{
    char password[DB_PASSWORD_MAX];
    char username[DB_USERNAME_MAX];
    bool remember_me;
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


/**
 *  `ew->db.ctx.client` will contain the client reference. 
 *  No need to pass client_t in this function.
 **/
// const char* 
// server_client_login_session_uuid(eworker_t* ew, 
//                                  const char* session_uuid);

// const char* 
// server_client_login_session(eworker_t* th, 
//                             client_t* client, 
//                             json_object* payload, 
//                             json_object* respond_json);

#endif // _SERVER_USER_LOGIN_H_
