#include "backend.h"
#include "server.h"

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

    http = http_new_resp(status, body, payload_len);
    http_add_cross_origin_headers(client, http);

    json_object_object_foreach(json_headers, key, val) {
        const char* val_string;

        if (json_object_is_type(val, json_type_string))
            val_string = json_object_get_string(val);
        else
            val_string = json_object_to_json_string_ext(val, JSON_C_TO_STRING_NOSLASHESCAPE);

        http_add_header(http, key, val_string);
    }

    http_send(client, http);

    http_free(http);
}

static void
backend_service_send(server_t* server, const char* subject, json_object* json)
{
    u64 len;
    const char* str;

    str = json_object_to_json_string_length(json, JSON_C_TO_STRING_NOSLASHESCAPE, &len);

    verbose("NATS PUB: Subject='%s', Data=%s\n", subject, str);

    natsConnection_PublishRequest(server->nats.conn, subject, server->nats.subj_http, str, len);
}

void 
backend_send_http(server_t* server, const char* subject, client_t* client, http_t* http, u32 user_id)
{
    const http_header_t* content_type = http_get_header(http, "Content-Type");
    json_object* json = json_object_new_object();
    json_object* params_json = json_object_new_object();
    json_object_object_add(json, "method", json_object_new_string(http->req.method));
    json_object_object_add(json, "fd", json_object_new_int(client->addr.sock));
    json_object_object_add(json, "user_id", json_object_new_uint64(user_id));
    json_object_object_add(json, "path", json_object_new_string(http->req.url));
    json_object_object_add(json, "params", params_json);

    for (u32 i = 0; i < http->n_params; i++)
    {
        http_header_t* param = http->params + i;
        json_object_object_add(params_json, param->name, json_object_new_string(param->val));
    }

    if (http->body)
    {
        if (content_type && 
            strncmp(content_type->val, "application/json", HTTP_HEAD_VAL_LEN) == 0)
        {
            json_object_object_add(json, "body", json_tokener_parse(http->body));
        }
        else 
        {
            server_http_resp(client, HTTP_CODE_BAD_REQ);
            return;
        }
    }
    else
        json_object_object_add(json, "body", json_object_new_null());

    backend_service_send(server, subject, json);

    json_object_put(json);
}
