#ifndef _SERVER_USER_SESSION_H_
#define _SERVER_USER_SESSION_H_

#include "common.h"

typedef struct client client_t;

typedef struct session
{
    u32 session_id;
    u32 user_id;
    i32 timerfd;

    struct session* next;
    struct session* prev;
} session_t;

session_t*      server_new_user_session(server_t* server, client_t* client);
session_t*      server_get_user_session(server_t* server, u32 session_id);
session_t*      server_get_user_session_uid(server_t* server, u32 user_id);
void            server_del_user_session(server_t* server, session_t* session);

#endif // _SERVER_USER_SESSION_H_
