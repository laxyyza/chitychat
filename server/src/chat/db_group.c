#include "chat/db_pipeline.h"
#include "chat/db_group.h"
#include "chat/db_def.h"
#include "chat/db.h"
#include <libpq-fe.h>

static void
db_get_groups_result(UNUSED eworker_t* ew, PGresult* res, ExecStatusType status, dbcmd_ctx_t* ctx)
{
    dbgroup_t* groups = NULL;
    i32 rows;

    if (status == PGRES_TUPLES_OK)
    {
        rows = PQntuples(res);
        if (rows)
        {
            groups = calloc(rows, sizeof(dbgroup_t));
            for (i32 i = 0; i < rows; i++)
                db_row_to_group(groups + i, res, i);
        }
        ctx->data = groups;
        ctx->data_size = rows;
        ctx->ret = DB_ASYNC_OK;
    }
    else
    {
        error("async get_groups: %s\n",
              PQresultErrorMessage(res));
        ctx->ret = DB_ASYNC_ERROR;
    }
}

bool 
db_async_get_group(server_db_t* db, u32 group_id, dbcmd_ctx_t* ctx)
{
    const char* sql = "SELECT * FROM Groups WHERE group_id = $1::int;";
    i32 ret;
    char group_id_str[DB_INTSTR_MAX];
    const char* vals[1] = {
        group_id_str
    };
    const i32 lens[1] = {
        snprintf(group_id_str, DB_INTSTR_MAX, "%u", group_id)
    };
    const i32 formats[1] = {0};
    ctx->exec_res = db_get_groups_result;
    ret = db_async_params(db, sql, 1, vals, lens, formats, ctx);
    
    return ret == 1;
}

bool 
db_async_get_user_groups(server_db_t* db, u32 user_id, dbcmd_ctx_t* ctx)
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
    ctx->exec_res = db_get_groups_result;
    ret = db_async_params(db, db->cmd->select_user_groups, 1, vals, lens, formats, ctx);
    return ret == 1;
}
