#include "server.h"

#include <string.h>
#include <errno.h>
#include <json-c/json.h>

bool        server_load_config(server_t* server, const char* config_path)
{
#define JSON_GET(x) json_object_object_get(config, x)

    json_object* config;
    json_object* root_dir;
    json_object* addr_ip;
    json_object* addr_port;
    json_object* addr_version;
    json_object* database;

    config = json_object_from_file(config_path);
    if (!config)
    {
        fatal(json_util_get_last_err());
        return false;
    }

    root_dir = JSON_GET("root_dir");
    const char* root_dir_str = json_object_get_string(root_dir);
    strncpy(server->conf.root_dir, root_dir_str, CONFIG_PATH_LEN);

    addr_ip = JSON_GET("addr_ip");
    const char* addr_ip_str = json_object_get_string(addr_ip);
    strncpy(server->conf.addr_ip, addr_ip_str, INET6_ADDRSTRLEN);

    addr_port = JSON_GET("addr_port");
    server->conf.addr_port = json_object_get_int(addr_port);

    addr_version = JSON_GET("addr_version");
    const char* addr_version_str = json_object_get_string(addr_version);
    if (!strcmp(addr_version_str, "ipv4"))
        server->conf.addr_version = IPv4;
    else if (!strcmp(addr_version_str, "ipv6"))
        server->conf.addr_version = IPv6;
    else
        warn("Config: addr_version: \"%s\"? Default to IPv4\n", addr_version_str);
    //strncpy(server->conf.addr_version, addr_version_str, CONFIG_ADDR_VRESION_LEN);

    database = JSON_GET("database");
    const char* database_str = json_object_get_string(database);
    strncpy(server->conf.database, database_str, CONFIG_PATH_LEN);

    json_object_put(root_dir);
    json_object_put(addr_ip);
    json_object_put(addr_port);
    json_object_put(addr_version);
    json_object_put(database);
    json_object_put(config);

    info("config:\n\troot_dir: %s\n\taddr_ip: %s\n\taddr_port: %u\n\taddr_version: %s\n\tdatabase: %s\n",
        server->conf.root_dir,   
        server->conf.addr_ip,   
        server->conf.addr_port,   
        (server->conf.addr_version == IPv4) ? "IPv4" : "IPv6",   
        server->conf.database
    );

    return true;
}

static bool server_init_socket(server_t* server)
{
    int domain;

    if (server->conf.addr_version == IPv4)
    {
        domain = AF_INET;
        server->addr_len = sizeof(struct sockaddr_in);
        server->addr_in.sin_family = AF_INET;
        server->addr_in.sin_port = htons(server->conf.addr_port);

        if (!strncmp(server->conf.addr_ip, "any", 3))
            server->addr_in.sin_addr.s_addr = INADDR_ANY;
        else
            server->addr_in.sin_addr.s_addr = inet_addr(server->conf.addr_ip);
    }
    else
    {
        domain = AF_INET6;
        server->addr_len = sizeof(struct sockaddr_in6);
        server->addr_in6.sin6_family = AF_INET6;
        server->addr_in6.sin6_port = htons(server->conf.addr_port);
        if (!strncmp(server->conf.addr_ip, "any", 3))
            server->addr_in6.sin6_addr = in6addr_any;
        else
        {
            if (inet_pton(AF_INET6, server->conf.addr_ip, &server->addr_in6.sin6_addr) <= 0)
            {
                fatal("inet_pton: Invalid IPv6 address: '%s'\n", server->conf.addr_ip);
                return false;
            }
        }
    }

    server->addr = (struct sockaddr*)&server->addr_in;

    server->sock = socket(domain, SOCK_STREAM, 0);
    if (server->sock == -1)
    {
        fatal("socket: %s\n", strerror(errno));
        return false;
    }

    int opt = 1;
    if (setsockopt(server->sock, SOL_SOCKET, SO_REUSEADDR, &opt, server->addr_len) == -1)
        error("setsockopt: %s\n", strerror(errno));

    if (bind(server->sock, server->addr, server->addr_len) == -1)
    {   
        fatal("bind: %s\n", strerror(errno));
        return false;
    }

    if (listen(server->sock, 10) == -1)
    {
        fatal("listen: %s\n", strerror(errno)); 
        return false;
    }

    return true;
}

server_t*   server_init(const char* config_path)
{
    server_t* server = calloc(1, sizeof(server_t));

    if (!server_load_config(server, config_path))
        goto error;

    if (!server_init_socket(server))
        goto error;

    if (!server_init_epoll(server))
        goto error;

    return server;
error:;
    server_cleanup(server);
    return NULL;
}

void        server_run(server_t* server)
{

}

void        server_cleanup(server_t* server)
{
    if (!server)
        return;

    free(server);
}