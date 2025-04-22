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
#include "server_ht.h"
#include "server_signal.h"
#include "db/db.h"
#include "backend_route.h"
#include "server_nats.h"
#include "server_redis.h"

#define SERVER_NAME "ChityChat"

#define CONFIG_PATH_LEN 512
#define CONFIG_USER_LEN_MAX 128
#define CONFIG_ADDR_VRESION_LEN 10

#define MAX_SESSIONS 10
#define MAX_EP_EVENTS 64

enum client_recv_status
{
    RECV_OK,
    RECV_DISCONNECT,
    RECV_ERROR
};

typedef struct server_config
{
    char root_dir[CONFIG_PATH_LEN];
    char img_dir[CONFIG_PATH_LEN];
    char vid_dir[CONFIG_PATH_LEN];
    char file_dir[CONFIG_PATH_LEN];
    char addr_ip[INET6_ADDRSTRLEN];
    uint16_t addr_port;
    enum ip_version addr_version;
    bool fork;
    i32  thread_pool;

    const char* redis_ip;
    u16 redis_port;

    bool retry_db_connect;
    bool disable_tls;

    bool sql_time;  // Display SQL Query Times.
    bool event_time;// Display Event Times.
} server_config_t;
 
typedef struct server
{
    struct {
        i32 epfd;       /* epoll fd */
        i32 eventfd;    /* eventfd (used to wake up threads from epoll_wait()) */
        i32 sigfd;      /* signalfd */
    };
    server_config_t conf;
    server_db_commands_t db_commands;
    server_tm_t tm;
    magic_t magic_cookie;
    SSL_CTX* ssl_ctx;

    i32 domain;
    struct sockaddr* addr;
    union {
        struct sockaddr_in addr_in;
        struct sockaddr_in6 addr_in6;
    };
    socklen_t addr_len;

    server_ght_t event_ht;
    server_ght_t client_ht;
    server_ght_t user_ht;
    server_ght_t chat_cmd_ht;
    server_event_t* server_event;
    eworker_t* main_ew;

    server_nats_t nats;

    bool running;
} server_t;

void server_run(server_t* server);
void server_cleanup(server_t* server);
i32  server_print_sockerr(i32 fd);

ssize_t     server_send(client_t* client, const void* buf, size_t len);
ssize_t     server_recv(client_t* client, void* buf, size_t len);

#endif // _SERVER_H_
