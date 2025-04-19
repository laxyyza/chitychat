#include "server.h"
#include "chat/db.h"
#include "chat/db_remember_token.h"
#include "chat/db_pipeline.h"

static bool
set_token_expire_time(remember_token_t* rt, const char* expires_at)
{
    struct tm tm;
    time_t t;
    struct tm* gmt;

    memset(&tm, 0, sizeof(struct tm));

    if (strptime(expires_at, "%Y-%m-%d %H:%M:%S", &tm) == NULL) 
    {
        error("strptime failed for '%s': %s\n", expires_at, ERRSTR);
        return false;
    }

    t = timegm(&tm);
    gmt = gmtime(&t);

    strftime(rt->expires, TOKEN_EXPIRE_STR_LEN, "%a, %d %b %Y %H:%M:%S GMT", gmt);

    return true;
}

static void
db_insert_token_result(UNUSED eworker_t* ew, PGresult* res, ExecStatusType status, dbcmd_ctx_t* ctx)
{
    const char* expires_at;
    remember_token_t* rt = ctx->data;

    if (status == PGRES_TUPLES_OK)
    {
        expires_at = PQgetvalue(res, 0, 0);

        if (set_token_expire_time(rt, expires_at))
            ctx->ret = DB_ASYNC_OK;
        else
            ctx->ret = DB_ASYNC_ERROR;
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
    ret = db_async_params(db, &db->cmd->insert_remember_token, 3, vals, lens, formats, ctx);
    return ret == 1;
}

static void 
db_select_token_result(UNUSED eworker_t* ew, PGresult* res, ExecStatusType status, dbcmd_ctx_t* ctx)
{
    char* endptr;
    ctx->ret = DB_ASYNC_ERROR;

    if (status == PGRES_TUPLES_OK)
    {
        if (PQntuples(res) == 0)
        {
            ctx->ret = DB_ASYNC_ERROR;
            return;
        }

        ctx->ret = DB_ASYNC_OK;
        const char* token_id_str = PQgetvalue(res, 0, 0);
        if (token_id_str)
            ctx->param.remember_token.token_id = strtoul(token_id_str, &endptr, 10);
        else
            warn("token_id_str is NULL\n");

        const char* user_id_str = PQgetvalue(res, 0, 1);
        if (user_id_str)
            ctx->param.remember_token.user_id = strtoul(user_id_str, &endptr, 10);
        else
            warn("user_id_str is NULL\n");
    }
    else
        error("select_remember_token: %s\n", PQresultErrorMessage(res));
}

bool
db_async_select_remember_token(server_db_t* db, const u8* token_hash, dbcmd_ctx_t* ctx)
{
    i32 ret;
    const char* vals[1] = {
        (const char*)token_hash,
    };
    const i32 formats[1] = {
        1,
    };
    const i32 lens[1] = {
        TOKEN_LEN,
    };
    ctx->exec_res = db_select_token_result;
    ret = db_async_params(db, &db->cmd->select_remember_token, 1, vals, lens, formats, ctx);
    return ret == 1;
}

static void 
db_update_token_result(UNUSED eworker_t* ew, PGresult* res, ExecStatusType status, dbcmd_ctx_t* ctx)
{
    remember_token_t* rt = ctx->data;

    if (status == PGRES_TUPLES_OK)
    {
        const char* expires_at = PQgetvalue(res, 0, 0);

        if (expires_at)
        {
            set_token_expire_time(rt, expires_at);
            ctx->ret = DB_ASYNC_OK;
        }
        else
        {
            warn("expires_at NULL!\n");
            ctx->ret = DB_ASYNC_ERROR;
        }
    }
    else
    {
        ctx->ret = DB_ASYNC_ERROR;
        error("update_remember_token: %s\n", PQresultErrorMessage(res));
    }
}

bool
db_async_update_remember_token(server_db_t* db, remember_token_t* rt, dbcmd_ctx_t* ctx)
{
    i32 ret;
    const char* expires_at = "30 Days"; // TODO: Not hard code expire time.
    char token_id_str[DB_INTSTR_MAX];
    const char* vals[3] = {
        (const char*)rt->token_hash,
        expires_at,
        token_id_str
    };
    const i32 formats[3] = {
        1,
        0,
        0
    };
    const i32 lens[3] = {
        TOKEN_LEN,
        strlen(expires_at),
        snprintf(token_id_str, DB_INTSTR_MAX, "%u", rt->token_id)
    };
    ctx->exec_res = db_update_token_result;
    ctx->data = rt;
    ret = db_async_params(db, &db->cmd->update_remember_token, 3, vals, lens, formats, ctx);
    return ret == 1;
}
