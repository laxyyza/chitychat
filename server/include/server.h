#ifndef _SERVER_H_
#define _SERVER_H_

#include "common.h"
#include "server_net.h"
#include "server_client.h"
#include "server_tm.h"
#include "server_events.h"
#include "server_init.h"
#include "server_crypt.h"
#include "server_timer.h"
#include "server_util.h"
#include "server_http.h"
#include "server_websocket.h"
#include "chat/user_file.h"
#include "chat/db.h"
#include "chat/upload_token.h"
#include "chat/user_session.h"

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
    i32  thread_pool;

    const char* sql_schema;
    const char* sql_insert_user;
    const char* sql_select_user;

    const char* sql_insert_group;
    const char* sql_select_group;

    const char* sql_insert_groupmember_code;
    const char* sql_select_groupmember;

    const char* sql_insert_msg;
    const char* sql_select_msg;

    const char* sql_update_user;

    const char* sql_insert_userfiles;
} server_config_t;
 
typedef struct server
{
    i32 sock;
    i32 epfd;
    server_config_t conf;
    server_db_commands_t db_commands;
    server_tm_t tm;
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

void server_run(server_t* server);
void server_cleanup(server_t* server);
void server_print_sockerr(i32 fd);
void server_ep_event(server_thread_t* th, const server_job_t* job);

ssize_t     server_send(const client_t* client, const void* buf, size_t len);
ssize_t     server_recv(const client_t* client, void* buf, size_t len);

#endif // _SERVER_H_
