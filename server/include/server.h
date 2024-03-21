#ifndef _SERVER_H_
#define _SERVER_H_

#include "common.h"
#include "server_client.h"
#include "server_log.h"
#include "server_net.h"
#include "server_http.h"
#include "server_websocket.h"
#include "server_db.h"
#include "server_util.h"
#include "server_crypt.h"
#include "server_user_file.h"
#include "server_init.h"
#include "server_client_sesson.h"
#include "server_upload_token.h"

#define SERVER_CONFIG_PATH "server/config.json"

#define CONFIG_PATH_LEN 512
#define CONFIG_ADDR_VRESION_LEN 10

#define MAX_SESSIONS 10
#define MAX_TMP_MSGS 10

enum client_recv_status
{
    RECV_OK,
    RECV_DISCONNECT,
    RECV_ERROR
};

typedef struct 
{
    char root_dir[CONFIG_PATH_LEN];
    char img_dir[CONFIG_PATH_LEN];
    char vid_dir[CONFIG_PATH_LEN];
    char file_dir[CONFIG_PATH_LEN];
    char addr_ip[INET6_ADDRSTRLEN];
    uint16_t addr_port;
    enum ip_version addr_version;
    char database[CONFIG_PATH_LEN];
    bool secure_only;
    bool fork;

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
    const char* sql_insert_userfiles;
} server_config_t;

enum se_status 
{
    SE_OK,
    SE_END,
    SE_ERROR,
};

// TODO: Use server_event_t
typedef enum se_status (*se_close_callback_t)(server_t* server, void* data);
typedef enum se_status (*se_read_callback_t)(server_t* server, const u32 ep_events, void* data);

typedef struct server_event
{
    i32 fd;
    void* data;
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
    magic_t magic_cookie;

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
    tmp_msg_t tmp_msg[MAX_TMP_MSGS];

    SSL_CTX* ssl_ctx;

    bool running;
} server_t;

void            server_run(server_t* server);
void            server_cleanup(server_t* server);

server_event_t* server_new_event(server_t* server, i32 fd, void* data, 
                                se_read_callback_t read_callback, 
                                se_close_callback_t close_callback);
server_event_t* server_get_event(server_t* server, i32 fd);
void            server_del_event(server_t* server, server_event_t* se);

int         server_ep_addfd(server_t* server, i32 fd);
int         server_ep_delfd(server_t* server, i32 fd);

ssize_t     server_send(const client_t* client, const void* buf, size_t len);
ssize_t     server_recv(const client_t* client, void* buf, size_t len);

#endif // _SERVER_H_