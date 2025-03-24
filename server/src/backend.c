#include "backend.h"
#include "server.h"
#include <netinet/tcp.h>
#include "server_events.h"

#define PORT 6000
#define BUF_SIZE 4096

/*
>>> http client sends:
    HTTP GET /friends
    Cookie: session_id=session-id-1234

>>> server sends this to backend service:
{
    "type": "GET",
    "fd": 41,
    "user_id": 51
    "path": "/friends"
}

>>> backend service responds back with:
{
    "type": "GET",
    "fd": 41,
    "status": 200, 
    "headers": {
        "Content-Type": "application/json",
        "Content-Length": 69420
    },
    "payload": {
        "friends": {51, 61123, 612, 331}
    }
}

>>> server sends HTTP 200 OK 
    Content-Type: "application/json"
    Content-Length: 69420

    {
        "friends": [51, 61123, 612, 331]
    }

*/

/*

{
    "type": "GET" | "POST" | "WS" (from HTTP GET, POST or WebSockets)
    "fd": 44, (only for type GET/POST)
    "status": 200, (only for type GET/POST)
    "user_ids": [14] (only for type WS)
    "headers": { (only for type GET/POST)
        "Content-Type": "application/json"
    },
    "payload": {
        "friends": [451, 514, 31]
    }
}

*/
static enum se_status 
registered_backend_read(eworker_t* ew, backend_service_t* service, json_object* json)
{
    // TODO
}

/*

{
    "register_paths": ["/friends"],
    "register_cmds": ["hub_msg"],
    "key": "secret-backend-key-1234" (future me problem to implement)
}

*/
static enum se_status 
unknown_backend_read(eworker_t* ew, backend_service_t* service, json_object* json)
{
    // TODO
}

static enum se_status
se_backend_read(UNUSED eworker_t* ew, server_event_t* ev)
{
    backend_service_t* service = ev->data;
    char buffer[BUF_SIZE];
    char* offset = buffer;
    i64 bytes_recv;

    if ((bytes_recv = recv(ev->fd, buffer, BUF_SIZE, 0)) == -1)
    {
        error("backend recv: %s\n", ERRSTR);
        return SE_ERROR;
    }
    else if (bytes_recv == 0)
        return SE_CLOSE;

    const u32 size = *(u32*)offset;
    const char* json_str = offset + sizeof(u32);
    json_object* json = json_tokener_parse(json_str);

    if (size != (bytes_recv - sizeof(u32)))
        warn("NOT SAME. size: %u, bytes_recv: %lld\n", size, bytes_recv);


    if (service->registered)
        return registered_backend_read(ew, service, json);
    else 
        return unknown_backend_read(ew, service, json);
}

static enum se_status
se_backend_close(eworker_t* ew, server_event_t* ev)
{
    backend_service_t* service = ev->data;

    warn("Backend '%s' (%s, fd: %d) disconnected!\n", service->name, service->addr.ip_str, service->addr.sock);

    close(service->addr.sock);

    server_ght_del(&ew->server->bservices_ht, service->addr.sock);

    return SE_OK;
}

static enum se_status
se_accept_backend(eworker_t* ew, server_event_t* ev)
{
    backend_service_t* service = calloc(1, sizeof(backend_service_t));

    service->addr.len = sizeof(struct sockaddr_in6);
    service->addr.version = IPv6;
    service->addr.addr_ptr = (struct sockaddr*)&service->addr.ipv6;
    service->addr.sock = accept(ev->fd, service->addr.addr_ptr, &service->addr.len);
    if (service->addr.sock == -1)
    {
        error("accept backend: %s\n", ERRSTR);
        free(service);
        return SE_ERROR;
    }

    inet_ntop(AF_INET6, &service->addr.ipv6.sin6_addr, service->addr.ip_str, INET6_ADDRSTRLEN);

    server_ght_insert(&ew->server->bservices_ht, service->addr.sock, service);

    server_new_event(ew->server, service->addr.sock, service, se_backend_read, se_backend_close);

    info("Backend (fd: %d) %s connected.\n", service->addr.sock, service->addr.ip_str);

    return SE_OK;
}

bool 
backend_socket_init(server_t* server)
{
    i32 sock;
    struct sockaddr_in6 addr;
    socklen_t socklen;
    i32 opt;

    socklen = sizeof(struct sockaddr_in6);
    addr.sin6_family = AF_INET6;
    addr.sin6_port = htons(PORT);
    addr.sin6_addr = in6addr_loopback;

    if ((sock = socket(AF_INET6, SOCK_STREAM, 0)) == -1)
    {
        fatal("socket");
        return false;
    }

    opt = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(i32)) == -1)
        error("setsockopt SO_REUSEADDR: %s\n", ERRSTR);

    opt = 1;
    if (setsockopt(sock, SOL_TCP, TCP_NODELAY, &opt, sizeof(i32)) == -1)
        error("setsockopt TCP_NODELAY: %s\n", ERRSTR);

    if (bind(sock, &addr, socklen) == -1)
    {
        fatal("backend socket bind: %s\n", ERRSTR);
        return false;
    }

    if (listen(sock, 10) == -1)
    {
        fatal("listen: %s\n", ERRSTR);
        return false;
    }

    server_new_event(server, sock, NULL, se_accept_backend, NULL);

    return true;
}

void 
backend_services_close(UNUSED server_t* server)
{
    
}
