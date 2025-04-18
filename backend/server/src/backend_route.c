#include "backend_route.h"
#include "server_client.h"
#include "server.h"
#include "backend.h"

// static const char* 
// do_get_session(eworker_t* ew, dbcmd_ctx_t* ctx)
// {
//     dbsession_t* session = ctx->data;
//     const char* subject = ctx->param.session_route.subject;
//     http_t* http = ctx->param.session_route.http;
//
//     if (ctx->ret == DB_ASYNC_ERROR)
//     {
//         server_http_resp(ctx->client, HTTP_CODE_UNAUTHORIZED);
//         http_free(http);
//         free((void*)subject);
//         return "Invalid session ID";
//     }
//
//     backend_send_http(ew->server, subject, ctx->client, http, session->user_id);
//
//     http_free(http);
//     free((void*)subject);
//
//     return NULL;
// }

static void 
after_get_session_user_id(server_t* server, get_session_data_t* data)
{
    client_t* client = data->client;
    http_t* http = data->http;
    const char* subject = data->subject;
    u32 user_id = data->user_id;

    if (data->found == false)
    {
        server_http_resp(client, HTTP_CODE_UNAUTHORIZED);
        goto cleanup;
    }

    backend_send_http(server, subject, client, http, user_id);
cleanup:
    http_free(http);
}

bool 
backend_route(eworker_t* ew, client_t* client, http_t* http)
{
    const char* session;
    if ((session = http->cookies.session_uuid) == NULL)
    {
        server_http_resp(client, HTTP_CODE_UNAUTHORIZED);
        return true;
    }

    redis_cb_data_t* data = server_redis_get_cb_data(&ew->server->redis);
    data->data.http = http;
    data->data.client = client;
    data->data.user_id = 0;
    data->callback = after_get_session_user_id;
    strcpy(data->data.subject, "http");

    info("data: %p\n", data);

    char* nats_subject = data->data.subject;
    u32 i = strncpy_replace(nats_subject + 4, http->req.url, SUBJECT_LEN - 5, '/', '.');
    nats_subject[i + 4] = '.';
    strncat(nats_subject, http->req.method, SUBJECT_LEN - 1);

    ew->ignore_http_free = true;
    server_redis_get_session(ew->server, session, data);

    return true;
}
