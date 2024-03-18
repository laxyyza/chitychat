#ifndef _SERVER_CLIENT_SESSION_H_
#define _SERVER_CLIENT_SESSION_H_

#include "common.h"
#include "server_client.h"

typedef struct session
{
    u32 session_id;
    u32 user_id;
    i32 timerfd;

    struct session* next;
    struct session* prev;
} session_t;

session_t*      server_new_client_session(server_t* server, client_t* client);
session_t*      server_get_client_session(server_t* server, u32 session_id);
void            server_del_client_session(server_t* server, session_t* session);

#endif // _SERVER_CLIENT_SESSION_H_