#ifndef _SERVER_H_
#define _SERVER_H_

#include <stdint.h>
#include "server_client.h"
#include "server_log.h"
#include <linux/limits.h>
#include "server_net.h"
#include "common.h"
#include "server_http.h"
#include "server_websocket.h"
#include "server_db.h"
#include "server_util.h"

#define SERVER_CONFIG_PATH "server/config.json"

#define CONFIG_PATH_LEN 512
#define CONFIG_ADDR_VRESION_LEN 10

#define MAX_SESSIONS 10

enum client_recv_status
{
    RECV_OK,
    RECV_DISCONNECT,
    RECV_ERROR
};

typedef struct 
{
    char root_dir[CONFIG_PATH_LEN];
    char addr_ip[INET6_ADDRSTRLEN];
    uint16_t addr_port;
    enum ip_version addr_version;
    char database[CONFIG_PATH_LEN];

    const char* sql_schema;
    const char* sql_insert_user;
    const char* sql_select_user;
    const char* sql_delete_user;
    const char* sql_insert_group;
    const char* sql_select_group;
    const char* sql_delete_group;
    const char* sql_insert_groupmember;
    const char* sql_select_groupmember;
    const char* sql_delete_groupmember;
    const char* sql_insert_msg;
    const char* sql_select_msg;
    const char* sql_delete_msg;
    const char* sql_update_user;
} server_config_t;


typedef struct
{
    u32 session_id;
    u32 user_id;
} session_t;

typedef struct 
{
    u64 token;
    u32 user_id;
} upload_token_t;

typedef struct server
{
    int sock;
    int epfd;
    server_config_t conf;
    server_db_t db;

    struct sockaddr* addr;
    union {
        struct sockaddr_in addr_in;
        struct sockaddr_in6 addr_in6;
    };
    socklen_t addr_len;

    client_t* client_head;

    session_t sessions[MAX_SESSIONS];
    upload_token_t upload_tokens[MAX_SESSIONS];

    SSL_CTX* ssl_ctx;

    bool running;
} server_t;

server_t*       server_init(const char* config_path);
void            server_run(server_t* server);
void            server_cleanup(server_t* server);
upload_token_t* server_find_upload_token(server_t* server, u64 id);

int         server_ep_addfd(server_t* server, i32 fd);
int         server_ep_delfd(server_t* server, i32 fd);

ssize_t     server_send(const client_t* client, const void* buf, size_t len);
ssize_t     server_recv(const client_t* client, void* buf, size_t len);

#endif // _SERVER_H_