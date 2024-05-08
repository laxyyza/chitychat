#ifndef _SERVER_CLIENT_H_
#define _SERVER_CLIENT_H_

#include "common.h"
#include "server_net.h"
#include "chat/user_session.h"
#include "chat/user.h"

#define USERNAME_MAX 50
#define DISPLAYNAME_MAX 50

#define CLIENT_STATE_SHORT_LIVE      0x0000
#define CLIENT_STATE_UPGRADE_PENDING 0x0001
#define CLIENT_STATE_WEBSOCKET       0x0002
#define CLIENT_STATE_KEEP_ALIVE      0x0004
#define CLIENT_STATE_LOGGED_IN       0x0008

#define CLIENT_ERR_NONE  00
#define CLIENT_ERR_SSL   01

#define CLIENT_RECV_PAGE             4096
#define CLIENT_MAX_ERRORS 3

typedef struct 
{
    u8*     data;
    size_t  data_size;
    size_t  offset;
    bool    busy;
    http_t* http;
} recv_buf_t;

typedef struct client
{
    net_addr_t  addr;
    u16         state;
    u16         err;
    SSL*        ssl;
    dbuser_t*   dbuser;
    session_t*  session;
    recv_buf_t  recv;
    pthread_mutex_t ssl_mutex;
} client_t;

client_t*   server_accept_client(server_thread_t* th);
int         server_client_ssl_handsake(server_t* server, client_t* client);
client_t*   server_get_client_fd(server_t* server, i32 fd);
client_t*   server_get_client_user_id(server_t* server, u64 id);
void        server_free_client(server_thread_t* th, client_t* client);
void        server_get_client_info(client_t* client);
void        server_set_client_err(client_t* client, u16 err);

#endif // _SERVER_CLIENT_H_
