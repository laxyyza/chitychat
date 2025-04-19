#include "chat/db_remember_token_usage.h"
#include "chat/db_pipeline.h"
#include "chat/db_def.h"
#include "chat/db.h"

static void 
after_insert(UNUSED eworker_t* ew, PGresult* res, ExecStatusType status, UNUSED dbcmd_ctx_t* ctx)
{
    if (status == PGRES_FATAL_ERROR)
        error("insert_remember_token_usage: %s\n", PQresultErrorMessage(res));
    ctx->ret = (status == PGRES_COMMAND_OK) ? DB_ASYNC_OK : DB_ASYNC_ERROR;
}

static const char*
dummy_cb(UNUSED eworker_t* ew, UNUSED dbcmd_ctx_t* ctx)
{
    // NO-OP
    return NULL;
}

bool 
db_async_remember_token_usage(server_db_t* db, 
                                u32 token_id,
                                const char* user_agent, 
                                const char* ip_address)
{
    i32 ret;
    char token_id_str[DB_INTSTR_MAX];
    const char* vals[3] = {
        token_id_str,
        user_agent, 
        ip_address
    };
    const i32 formats[3] = {0};
    const i32 lens[3] = {
        snprintf(token_id_str, DB_INTSTR_MAX - 1, "%u", token_id),
        strlen(user_agent),
        strlen(ip_address)
    };
    dbcmd_ctx_t ctx = {
        .exec_res = after_insert,
        .exec = dummy_cb
    };
    ret = db_async_params(db, &db->cmd->insert_remember_token_usage, 3, vals, lens, formats, &ctx);
    return ret == 1;
}
