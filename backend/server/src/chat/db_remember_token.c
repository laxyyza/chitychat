#include "server.h"
#include "chat/db.h"
#include "chat/db_remember_token.h"
#include "chat/db_pipeline.h"

static void
db_insert_token_result(UNUSED eworker_t* ew, PGresult* res, ExecStatusType status, dbcmd_ctx_t* ctx)
{
    const char* expires_at;
    struct tm tm;
    time_t t;
    struct tm* gmt;
    remember_token_t* rt = ctx->data;

    if (status == PGRES_TUPLES_OK)
    {
        expires_at = PQgetvalue(res, 0, 0);
        memset(&tm, 0, sizeof(struct tm));

        if (strptime(expires_at, "%Y-%m-%d %H:%M:%S", &tm) == NULL) 
        {
            error("strptime failed for '%s': %s\n", expires_at, ERRSTR);
            ctx->ret = DB_ASYNC_ERROR;
            return;
        }

        t = timegm(&tm);
        gmt = gmtime(&t);

        strftime(rt->expires, TOKEN_EXPIRE_STR_LEN, "%a, %d %b %Y %H:%M:%S GMT", gmt);

        ctx->ret = DB_ASYNC_OK;
    }
    else
    {
        ctx->ret = DB_ASYNC_ERROR;
        error("insert_remember_token: %s\n", 
              PQresultErrorMessage(res));
    }
}

bool 
db_async_insert_remember_token(server_db_t* db, remember_token_t* rt, dbcmd_ctx_t* ctx)
{
    i32 ret;
    char user_id_str[DB_INTSTR_MAX];
    const char* expire_time = "30 Days";
    const char* vals[3] = {
        user_id_str,
        (const char*)rt->token_hash,
        expire_time
    };
    const i32 formats[3] = {
        0,
        1,
        0
    };
    const i32 lens[3] = {
        snprintf(user_id_str, DB_INTSTR_MAX, "%u", rt->user_id),
        TOKEN_LEN,
        strlen(expire_time)
    };
    ctx->exec_res = db_insert_token_result;
    ctx->data = rt;
    ret = db_async_params(db, db->cmd->insert_remember_token, 3, vals, lens, formats, ctx);
    return ret == 1;
}
