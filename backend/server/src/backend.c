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


*/
void
backend_read(server_t* server, json_object* json)
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
    client = server_ght_get(&server->client_ht, fd);
    if (client == NULL)
    {
        warn("Client fd:%d not found\n", fd);
        return;
    }

    status = json_object_get_int(json_status);
    body = json_object_to_json_string_length(json_body, JSON_C_TO_STRING_NOSLASHESCAPE, &payload_len);

    http = http_new_resp(status, "OK", body, payload_len);

    json_object_object_foreach(json_headers, key, val) {
        http_add_header(http, key, json_object_to_json_string(val));
    }

    http_send(client, http);

    http_free(http);
}

/*

{
    "register_paths": ["/friends"],
    "register_cmds": ["hub_msg"],
    "name": "cc_friends",
    "key": "secret-backend-key-1234" (future me problem to implement)
}

*/
// static enum se_status 
// unknown_backend_read(eworker_t* ew, backend_service_t* service, json_object* json)
// {
//     // TODO
//     json_object* json_register_paths = json_object_object_get(json, "register_paths");
//     json_object* json_register_cmds = json_object_object_get(json, "register_cmds");
//     json_object* json_name = json_object_object_get(json, "name");
//     const char* name;

//     if (json_register_paths == NULL && json_register_cmds == NULL) 
//     {
//         warn("Service %s has no register keys!\n", service->addr.ip_str);
//         return SE_CLOSE;
//     }

//     if (json_name == NULL)
//     {
//         warn("Service %s has no name!\n", service->addr.ip_str);
//         return SE_CLOSE;
//     }

//     name = json_object_get_string(json_name);
//     if (name == NULL)
//     {
//         warn("Service %s json_object_get_string for 'name' is NULL!\n", service->addr.ip_str);
//         return SE_CLOSE;
//     }

//     strncpy(service->name, name, SERVICE_NAME_MAX);

//     if (json_register_paths)
//     {
//         u32 len = json_object_array_length(json_register_paths);
//         for (u32 i = 0; i < len; i++)
//         {
//             json_object* json_path = json_object_array_get_idx(json_register_paths, i);
//             if (json_path) 
//             {
//                 const char* path = json_object_get_string(json_path);
//                 if (path)
//                     backend_route_add(&ew->server->backend_routes, path, service);
//             }
//         }
//     }

//     service->registered = true;
//     backend_service_send(service, json);

//     return SE_OK;
// }

// static enum se_status
// se_backend_read(UNUSED eworker_t* ew, server_event_t* ev)
// {
//     enum se_status ret;
//     backend_service_t* service = ev->data;
//     char buffer[BUF_SIZE];
//     char* offset = buffer;
//     i64 bytes_recv;

//     if ((bytes_recv = recv(ev->fd, buffer, BUF_SIZE, 0)) == -1)
//     {
//         error("backend recv: %s\n", ERRSTR);
//         return SE_ERROR;
//     }
//     else if (bytes_recv == 0)
//         return SE_CLOSE;

//     const u32 size = *(u32*)offset;
//     const char* json_str = offset + sizeof(u32);
//     json_object* json = json_tokener_parse(json_str);

//     if (size != (bytes_recv - sizeof(u32)))
//         warn("NOT SAME. size: %u, bytes_recv: %lld\n", size, bytes_recv);


//     if (service->registered)
//         ret = registered_backend_read(ew, service, json);
//     else 
//         ret = unknown_backend_read(ew, service, json);

//     json_object_put(json);

//     return ret;
// }

static void
backend_service_send(server_t* server, const char* subject, json_object* json)
{
    u64 len;
    const char* str;

    str = json_object_to_json_string_length(json, JSON_C_TO_STRING_NOSLASHESCAPE, &len);

    info("Sending: %s\n", str);

    natsConnection_PublishRequest(server->nats.conn, subject, server->nats.subj_http, str, len);
}

void 
backend_send_http(server_t* server, const char* subject, client_t* client, http_t* http, u32 user_id)
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

    backend_service_send(server, subject, json);

    json_object_put(json);
}
