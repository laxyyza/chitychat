#include "chat/db_userfile.h"
#include "chat/db_def.h"
#include "chat/db.h"
#include "chat/db_pipeline.h"
#include <libpq-fe.h>

static void
insert_userfiles_result(UNUSED eworker_t* ew, 
                        PGresult* res, ExecStatusType status, dbcmd_ctx_t* ctx)
{
    if (status == PGRES_COMMAND_OK)
        ctx->ret = DB_ASYNC_OK;
    else
    {
        error("Async insert user file: %s\n",
              PQresultErrorMessage(res));
        ctx->ret = DB_ASYNC_ERROR;
    }
}

bool 
db_async_insert_userfile(server_db_t* db, dbuser_file_t* file, dbcmd_ctx_t* ctx)
{
    i32 ret;
    char size_str[DB_INTSTR_MAX];

    const char* vals[3] = {
        file->hash,
        size_str,
        file->mime_type
    };
    const i32 lens[3] = {
        strnlen(file->hash, DB_PFP_HASH_MAX),
        snprintf(size_str, DB_INTSTR_MAX, "%lu", file->size),
        strnlen(file->mime_type, DB_MIME_TYPE_LEN)
    };
    const i32 formats[3] = {0};

    ctx->exec_res = insert_userfiles_result;
    ctx->data = file;
    ret = db_async_params(db, db->cmd->insert_userfiles, 3, vals, lens, formats, ctx);
    return ret == 1;
}

static void 
delete_userfile_result(UNUSED eworker_t* ew, 
                       PGresult* res, ExecStatusType status, dbcmd_ctx_t* ctx)
{
    if (status == PGRES_COMMAND_OK)
        ctx->ret = DB_ASYNC_OK;
    else
    {
        error("Async delete user file: %s\n",
              PQresultErrorMessage(res));
        ctx->ret = DB_ASYNC_ERROR;
    }
}

bool 
db_async_delete_userfile(server_db_t* db, const char* hash, dbcmd_ctx_t* ctx)
{
    const char* sql = "UPDATE UserFiles SET ref_count = ref_count  - 1 WHERE hash = $1::text;";
    i32 ret;
    const char* vals[1] = {
        hash
    };
    const i32 lens[1] = {
        strnlen(hash, DB_PFP_HASH_MAX)
    };
    const i32 formats[1] = {0};

    ctx->exec_res = delete_userfile_result;
    ret = db_async_params(db, sql, 1, vals, lens, formats, ctx);

    return ret == 1;
}

static void
userfile_refcount_result(UNUSED eworker_t* ew,
                         PGresult* res, ExecStatusType status, dbcmd_ctx_t* ctx)
{
    i32 ref_count;
    const char* ref_count_str;

    if (status == PGRES_TUPLES_OK)
    {
        ref_count_str = PQgetvalue(res, 0, 0);
        ref_count  = atoi(ref_count_str);
        ctx->data_size = ref_count;
        ctx->ret = DB_ASYNC_OK;
    }
    else
    {
        error("async select userfile ref_count: %s\n",
              PQresultErrorMessage(res));
        ctx->ret = DB_ASYNC_ERROR;
    }
}

bool 
db_async_userfile_refcount(server_db_t* db, const char* hash, dbcmd_ctx_t* ctx)
{
    const char* sql = "SELECT ref_count FROM UserFiles WHERE hash = $1::text;";
    i32 ret;
    const char* vals[1] = {
        hash
    };
    const i32 lens[1] = {
        strnlen(hash, DB_PFP_HASH_MAX)
    };
    const i32 formats[1] = {0};
    ctx->exec_res = userfile_refcount_result;
    ret = db_async_params(db, sql, 1, vals, lens, formats, ctx);
    return ret == 1;
}
