#include "chat/db.h"
#include "chat/db_user_session.h"
#include "chat/db_pipeline.h"

static void 
db_insert_session_result(UNUSED eworker_t* ew, PGresult* res, ExecStatusType status, dbcmd_ctx_t* ctx)
{
    ctx->ret = DB_ASYNC_ERROR;

    if (status == PGRES_TUPLES_OK)
    {
        if (PQntuples(res) == 0)
            return;
        ctx->ret = DB_ASYNC_OK;
        const char* session_id = PQgetvalue(res, 0, 0);
        strncpy(ctx->param.session_id, session_id, UUID_LEN - 1);
    }
    else
        error("insert_session: %s\n", PQresultErrorMessage(res));
}

bool 
db_async_insert_session(server_db_t* db, u32 user_id, dbcmd_ctx_t* ctx)
{
    i32 ret;
    char user_id_str[DB_INTSTR_MAX];
    const char* vals[1] = {
        user_id_str
    };
    const i32 lens[1] = {
        snprintf(user_id_str, DB_INTSTR_MAX, "%u", user_id)
    };
    const i32 formats[1] = {0};

    ctx->exec_res = db_insert_session_result;
    ret = db_async_params(db, db->cmd->insert_session, 1, vals, lens, formats, ctx);
    return ret == 1;
}

static void 
db_select_session_result(UNUSED eworker_t* ew, PGresult* res, ExecStatusType status, dbcmd_ctx_t* ctx)
{
    dbsession_t* session;
    char* endptr;

    ctx->ret = DB_ASYNC_ERROR;

    if (status == PGRES_TUPLES_OK && PQntuples(res) == 1)
    {
        ctx->ret = DB_ASYNC_OK;
        session = ctx->data;
        const char* user_id_str = PQgetvalue(res, 0, 0);
        if (user_id_str)
            session->user_id = strtoul(user_id_str, &endptr, 10);
    }
    else if (status == PGRES_FATAL_ERROR)
    {
        error("select_session: %s\n", PQresultErrorMessage(res));
    }
}

bool 
db_async_select_session(server_db_t* db, const dbsession_t* session, dbcmd_ctx_t* ctx)
{
    // TODO: Use Redis or any in-memory DB server for storing sessions.
    i32 ret;
    const char* vals[1] = {
        session->uuid
    };
    const i32 lens[1] = {
        UUID_LEN
    };
    const i32 formats[1] = {0};
    ctx->exec_res = db_select_session_result;
    ctx->data = (void*)session;
    ret = db_async_params(db, db->cmd->select_session, 1, vals, lens, formats, ctx);
    return ret == 1;
}
