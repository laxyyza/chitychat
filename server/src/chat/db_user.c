#include "chat/db.h"
#include "chat/db_pipeline.h"
#include <libpq-fe.h>

static void 
db_get_user_result(UNUSED eworker_t* ew, PGresult* res, ExecStatusType status, dbcmd_ctx_t* ctx)
{
    i32 rows;
    ctx->ret = DB_ASYNC_ERROR;
    dbuser_t* user = NULL;

    if (status == PGRES_TUPLES_OK)
    {
        rows = PQntuples(res);
        if (rows == 0)
            return;
        ctx->ret = DB_ASYNC_OK;
        user = calloc(1, sizeof(dbuser_t));
        db_row_to_user(user, res, 0);
    }
    else
        error("get_user: %s\n", PQresultErrorMessage(res));
    ctx->data = user;
}

static void
db_insert_user_result(UNUSED eworker_t* ew, PGresult* res, ExecStatusType status, dbcmd_ctx_t* ctx)
{
    dbuser_t* user = ctx->data;
    ctx->ret = DB_ASYNC_ERROR;
    const char* user_id_str;
    char* endptr;

    if (status == PGRES_TUPLES_OK)
    {
        user_id_str = PQgetvalue(res, 0, 0);
        user->user_id = strtoul(user_id_str, &endptr, 10);
        ctx->ret = DB_ASYNC_OK;
    }
    else
    {
        error("insert_user: %s\n", 
              PQresultErrorMessage(res));
    }
}

bool
db_async_get_user(server_db_t* db, u32 user_id, dbcmd_ctx_t* ctx)
{
    i32 ret;
    const char* sql = "SELECT * FROM Users WHERE user_id = $1::int;";
    char user_id_str[DB_INTSTR_MAX];
    const char* vals[1] = {
        user_id_str
    };
    const i32 lens[1] = {
        snprintf(user_id_str, DB_INTSTR_MAX, "%u", user_id)
    };
    const i32 formats[1] = {0};
    ctx->exec_res = db_get_user_result;
    ret = db_async_params(db, sql, 1, vals, lens, formats, ctx);
    return ret == 1;    
}

bool
db_async_get_user_username(server_db_t* db, const char* username, dbcmd_ctx_t* ctx)
{
    i32 ret;
    const char* sql = "SELECT * FROM Users WHERE username = $1::varchar(50);";
    const char* const vals[1] = {
        username
    };
    const i32 lens[1] = {
        strnlen(username, DB_USERNAME_MAX)
    };
    const i32 formats[1] = {0};
    ctx->exec_res = db_get_user_result;

    ret = db_async_params(db, sql, 1, vals, lens, formats, ctx);
    return ret == 1;
}

static void
db_get_user_json_result(UNUSED eworker_t* ew, PGresult* res, ExecStatusType status, dbcmd_ctx_t* ctx)
{
    const char* user_array_json;

    if (status == PGRES_TUPLES_OK)    
    {
        user_array_json = PQgetvalue(res, 0, 0);
        if (user_array_json)
        {
            ctx->param.str = user_array_json;
            ctx->ret = DB_ASYNC_OK;
            return;
        }
    }
    else if (status == PGRES_FATAL_ERROR)
        error("get_user_json_array: %s\n",
              PQresultErrorMessage(res));

    ctx->ret = DB_ASYNC_ERROR;
}

bool 
db_async_get_user_array(server_db_t* db, const char* json_array, dbcmd_ctx_t* ctx)
{
    i32 ret;
    const char* vals[1] = {
        json_array
    };
    const i32 lens[1] = {
        strlen(json_array)
    };
    const i32 formats[1] = {0};
    ctx->exec_res = db_get_user_json_result;

    ret = db_async_params(db, db->cmd->select_user_json, 1, vals, lens, formats, ctx);
    return ret == 1;
}

bool 
db_async_insert_user(server_db_t* db, const dbuser_t* user, dbcmd_ctx_t* ctx)
{
    i32 ret;
    const char* vals[4] = {
        user->username,
        user->displayname,
        (const char*)user->hash,
        (const char*)user->salt
    };
    const i32 formats[4] = {
        0, 
        0,
        1,
        1
    };
    const i32 lens[4] = {
        strnlen(user->username, DB_USERNAME_MAX),
        strnlen(user->displayname, DB_DISPLAYNAME_MAX),
        SERVER_HASH_SIZE,
        SERVER_SALT_SIZE
    };
    ctx->exec_res = db_insert_user_result;
    ctx->data = (void*)user;
    ret = db_async_params(db, db->cmd->insert_user, 4, vals, lens, formats, ctx);
    return ret == 1;
}
