#ifndef _SERVER_CLIENT_H_
#define _SERVER_CLIENT_H_

#include "server_net.h"
#include "common.h"

#define USERNAME_MAX 50
#define DISPLAYNAME_MAX 50

#define CLIENT_STATE_SHORT_LIVE      0x0000
#define CLIENT_STATE_UPGRADE_PENDING 0x0001
#define CLIENT_STATE_WEBSOCKET       0x0002
#define CLIENT_STATE_KEEP_ALIVE      0x0004

#define CLIENT_RECV_BUFFER           4096

typedef struct 
{
    u64     id;
    char    username[USERNAME_MAX];
} client_user_t;

typedef struct client
{
    net_addr_t addr;
    u16 state;

    client_user_t ws_user;

    u8 recv_buffer[CLIENT_RECV_BUFFER];
    size_t recv_buf_index;

    struct client* next;
    struct client* prev;
} client_t;

client_t*   server_new_client(server_t* server);
client_t*   server_get_client_fd(server_t* server, i32 fd);
client_t*   server_get_client_user_id(server_t* server, u64 id);
void        server_del_client(server_t* server, client_t* client);

#endif // _SERVER_CLIENT_H_