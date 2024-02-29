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

#define SERVER_CONFIG_PATH "server/config.json"

#define CONFIG_PATH_LEN 512
#define CONFIG_ADDR_VRESION_LEN 10

typedef struct 
{
    char root_dir[CONFIG_PATH_LEN];
    char addr_ip[INET6_ADDRSTRLEN];
    uint16_t addr_port;
    enum ip_version addr_version;
    char database[CONFIG_PATH_LEN];
} server_config_t;

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
    client_t* client_tail;

    bool running;
} server_t;

server_t*   server_init(const char* config_path);
void        server_run(server_t* server);
void        server_cleanup(server_t* server);

int         server_ep_addfd(server_t* server, i32 fd);
int         server_ep_delfd(server_t* server, i32 fd);

#endif // _SERVER_H_