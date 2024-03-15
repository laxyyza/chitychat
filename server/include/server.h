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
#include "server_crypt.h"

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
    bool secure_only;

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


typedef struct session
{
    u32 session_id;
    u32 user_id;
    i32 timerfd;

    struct session* next;
    struct session* prev;
} session_t;

typedef struct upload_token
{
    u32 token;
    u32 user_id;
    i32 timerfd;

    struct upload_token* next;
    struct upload_token* prev;
} upload_token_t;

enum se_status 
{
    SE_OK,
    SE_END,
    SE_ERROR,
};

// #define SE_READ_CALLBACK(name) enum se_status (*name)(server_t* server, const u32 ep_events, void* data)
// #define SE_CLOSE_CALLBACK(name) enum se_status (*name)(server_t* server, void* data)
typedef enum se_status (*se_close_callback_t)(server_t* server, void* data);
typedef enum se_status (*se_read_callback_t)(server_t* server, const u32 ep_events, void* data);

typedef struct server_event
{
    i32 fd;
    void* data;
    // SE_READ_CALLBACK(read);
    // SE_CLOSE_CALLBACK(close);
    se_read_callback_t read;
    se_close_callback_t close;

    struct server_event* next;
    struct server_event* prev;
} server_event_t;
 
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
    session_t* session_head;
    upload_token_t* upload_token_head;
    server_event_t* se_head;

    SSL_CTX* ssl_ctx;

    bool running;
} server_t;

server_t*       server_init(const char* config_path);
void            server_run(server_t* server);
void            server_cleanup(server_t* server);

server_event_t* server_new_event(server_t* server, i32 fd, void* data, 
                                se_read_callback_t read_callback, 
                                se_close_callback_t close_callback);
server_event_t* server_get_event(server_t* server, i32 fd);
void            server_del_event(server_t* server, server_event_t* se);

session_t*      server_new_client_session(server_t* server, client_t* client);
session_t*      server_get_client_session(server_t* server, u32 session_id);
void            server_del_client_session(server_t* server, session_t* session);

upload_token_t* server_new_upload_token(server_t* server, u32 user_id);
upload_token_t* server_get_upload_token(server_t* server, u32 token);
void server_del_upload_token(server_t* server, upload_token_t* upload_token);

int         server_ep_addfd(server_t* server, i32 fd);
int         server_ep_delfd(server_t* server, i32 fd);

ssize_t     server_send(const client_t* client, const void* buf, size_t len);
ssize_t     server_recv(const client_t* client, void* buf, size_t len);

#endif // _SERVER_H_