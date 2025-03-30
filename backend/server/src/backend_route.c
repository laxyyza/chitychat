#include "backend_route.h"
#include "server_client.h"
#include "server.h"
#include "backend.h"
#include "chat/db_user_session.h"

#define SUBJECT_MAX 512

static const char* 
do_get_session(eworker_t* ew, dbcmd_ctx_t* ctx)
{
    dbsession_t* session = ctx->data;
    const char* subject = ctx->param.session_route.subject;
    http_t* http = ctx->param.session_route.http;

    if (ctx->ret == DB_ASYNC_ERROR)
    {
        server_http_resp_error(ctx->client, HTTP_CODE_UNAUTHORIZED, HTTP_UNAUTHORIZED);
        http_free(http);
        free((void*)subject);
        return "Invalid session ID";
    }

    backend_send_http(ew->server, subject, ctx->client, http, session->user_id);

    http_free(http);
    free((void*)subject);

    return NULL;
}

static void 
backend_do_route(eworker_t* ew, const char* subject, client_t* client, http_t* http)
{
    if (http->session_uuid == NULL)
    {
        server_http_resp_error(client, HTTP_CODE_UNAUTHORIZED, HTTP_UNAUTHORIZED);
        return;
    }

    client_t* real_client = server_ght_get(&ew->server->client_by_session_ht, 
                                           server_ght_hash_uuid(http->session_uuid));

    if (real_client && real_client->dbuser)
    {
        backend_send_http(ew->server, subject, client, http, real_client->dbuser->user_id);
    }
    else
    {
        dbsession_t* session = calloc(1, sizeof(dbsession_t));
        strncpy(session->uuid, http->session_uuid, UUID_LEN - 1);

        dbcmd_ctx_t ctx = {
            .exec = do_get_session,
            .param.session_route.http = http,
            .param.session_route.subject = strndup(subject, SUBJECT_MAX)
        };
        ew->ignore_http_free = true;

        db_async_select_session(&ew->db, session, &ctx);
    }
}

bool 
backend_route(eworker_t* ew, client_t* client, http_t* http)
{
    char nats_subject[SUBJECT_MAX] = "http";
    u32 i = strncpy_replace(nats_subject + 4, http->req.url, SUBJECT_MAX - 5, '/', '.');
    nats_subject[i + 4] = '.';
    strncat(nats_subject, http->req.method, SUBJECT_MAX - 1);

    backend_do_route(ew, nats_subject, client, http);

    return true;
}