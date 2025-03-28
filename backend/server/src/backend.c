#include "backend.h"
#include "server.h"
#include <netinet/tcp.h>
#include "server_events.h"

#define PORT 6000
#define BUF_SIZE 4096

static void 
backend_service_send(backend_service_t* bs, json_object* json)
{
    size_t len64;
    void* buf;
    const char* json_str = json_object_to_json_string_length(json, 0, &len64);
    const u32 len = len64;
    u32 buf_len = sizeof(u32) + len;

    if (json_str == NULL) 
    {
        error("json_object_to_json_string_length returned NULL!\n");
        return;
    }

    info("Sending to bs:%s: %s\n", bs->name, json_str);

    buf = malloc(buf_len);
    memcpy(buf, &len, sizeof(u32));
    memcpy(buf + sizeof(u32), json_str, len);

    if (send(bs->addr.sock, buf, buf_len, 0) == -1)
    {
        error("backend_service_send (%s): %s\n", bs->addr.ip_str, ERRSTR);
    }

    free(buf);
}

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
    json_object* json_fd = json_object_object_get(json, "fd");
    json_object* json_headers = json_object_object_get(json, "headers");
    json_object* json_body = json_object_object_get(json, "body");
    json_object* json_status = json_object_object_get(json, "status");
    i32 fd;
    i32 status;
    const char* body;
    size_t payload_len;
    client_t* client;
    http_t* http;

    fd = json_object_get_int(json_fd);
    client = server_ght_get(&ew->server->client_ht, fd);
    if (client == NULL)
    {
        warn("bs:%s read: Client fd:%d not found\n", service->name, fd);
        return SE_OK;
    }

    status = json_object_get_int(json_status);
    body = json_object_to_json_string_length(json_body, JSON_C_TO_STRING_NOSLASHESCAPE | JSON_C_TO_STRING_PLAIN, &payload_len);

    http = http_new_resp(status, "OK", body, payload_len);

    json_object_object_foreach(json_headers, key, val) {
        http_add_header(http, key, json_object_to_json_string(val));
    }

    http_send(client, http);

    http_free(http);

    // TODO
    return SE_OK;
}

/*

{
    "register_paths": ["/friends"],
    "register_cmds": ["hub_msg"],
    "name": "cc_friends",
    "key": "secret-backend-key-1234" (future me problem to implement)
}

*/
static enum se_status 
unknown_backend_read(eworker_t* ew, backend_service_t* service, json_object* json)
{
    // TODO
    json_object* json_register_paths = json_object_object_get(json, "register_paths");
    json_object* json_register_cmds = json_object_object_get(json, "register_cmds");
    json_object* json_name = json_object_object_get(json, "name");
    const char* name;

    if (json_register_paths == NULL && json_register_cmds == NULL) 
    {
        warn("Service %s has no register keys!\n", service->addr.ip_str);
        return SE_CLOSE;
    }

    if (json_name == NULL)
    {
        warn("Service %s has no name!\n", service->addr.ip_str);
        return SE_CLOSE;
    }

    name = json_object_get_string(json_name);
    if (name == NULL)
    {
        warn("Service %s json_object_get_string for 'name' is NULL!\n", service->addr.ip_str);
        return SE_CLOSE;
    }

    strncpy(service->name, name, SERVICE_NAME_MAX);

    if (json_register_paths)
    {
        u32 len = json_object_array_length(json_register_paths);
        for (u32 i = 0; i < len; i++)
        {
            json_object* json_path = json_object_array_get_idx(json_register_paths, i);
            if (json_path) 
            {
                const char* path = json_object_get_string(json_path);
                if (path)
                    backend_route_add(&ew->server->backend_routes, path, service);
            }
        }
    }

    service->registered = true;
    backend_service_send(service, json);

    return SE_OK;
}

static enum se_status
se_backend_read(UNUSED eworker_t* ew, server_event_t* ev)
{
    enum se_status ret;
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
        ret = registered_backend_read(ew, service, json);
    else 
        ret = unknown_backend_read(ew, service, json);

    json_object_put(json);

    return ret;
}

static enum se_status
se_backend_close(eworker_t* ew, server_event_t* ev)
{
    backend_service_t* service = ev->data;

    backend_route_del(&ew->server->backend_routes, service);

    close(service->addr.sock);

    warn("Backend '%s' (%s, fd: %d) disconnected!\n", service->name, service->addr.ip_str, service->addr.sock);

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

    backend_route_init(&server->backend_routes);

    return true;
}

void 
backend_services_close(server_t* server)
{
    backend_route_deinit(&server->backend_routes);
}

void 
backend_send_http(UNUSED server_t* server, backend_service_t* bs, client_t* client, http_t* http, u32 user_id)
{
    const http_header_t* content_type = http_get_header(http, "Content-Type");
    json_object* json = json_object_new_object();
    json_object_object_add(json, "method", json_object_new_string(http->req.method));
    json_object_object_add(json, "fd", json_object_new_int(client->addr.sock));
    json_object_object_add(json, "user_id", json_object_new_uint64(user_id));
    json_object_object_add(json, "path", json_object_new_string(http->req.url));

    if (http->body)
    {
        if (content_type && 
            strncmp(content_type->val, "application/json", HTTP_HEAD_VAL_LEN) == 0)
        {
            json_object_object_add(json, "body", json_tokener_parse(http->body));
        }
        else 
        {
            server_http_resp_error(client, HTTP_CODE_BAD_REQ, HTTP_BAD_REQ);
            return;
        }
    }
    else
        json_object_object_add(json, "body", json_object_new_null());

    backend_service_send(bs, json);

    json_object_put(json);
}
