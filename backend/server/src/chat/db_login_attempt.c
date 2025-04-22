#include "chat/db_login_attempts.h"
#include "chat/db_pipeline.h"
#include "chat/db_def.h"
#include "chat/db.h"

static void 
after_insert(UNUSED eworker_t* ew, PGresult* res, ExecStatusType status, UNUSED dbcmd_ctx_t* ctx)
{
    if (status == PGRES_FATAL_ERROR)
        error("insert_login_attempt: %s\n", PQresultErrorMessage(res));
    ctx->ret = (status == PGRES_COMMAND_OK) ? DB_ASYNC_OK : DB_ASYNC_ERROR;
}

static const char*
dummy_cb(UNUSED eworker_t* ew, UNUSED dbcmd_ctx_t* ctx)
{
    // NO-OP
    return NULL;
}

bool 
db_async_login_attempt(server_db_t* db, 
                        const char* username, 
                        bool successful, 
                        const char* user_agent, 
                        const char* ip_address)
{
    i32 ret;
    const char* successful_str = (successful) ? "true" : "false";
    const char* vals[4] = {
        username, 
        successful_str, 
        user_agent, 
        ip_address
    };
    const i32 formats[4] = {0};
    const i32 lens[4] = {
        strlen(username),
        strlen(successful_str),
        strlen(user_agent),
        strlen(ip_address)
    };
    dbcmd_ctx_t ctx = {
        .exec_res = after_insert,
        .exec = dummy_cb
    };
    ret = db_async_params(db, &db->cmd->insert_login_attempt, 4, vals, lens, formats, &ctx);
    return ret == 1;
}
