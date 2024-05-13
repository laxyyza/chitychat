#include "chat/db_pipeline.h"
#include "chat/db_group.h"
#include "chat/db_def.h"
#include "chat/db.h"
#include "chat/group.h"
#include <libpq-fe.h>
#include <openssl/conf.h>

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

static void
db_get_group_member_ids_result(UNUSED eworker_t* ew, PGresult* res, ExecStatusType status, dbcmd_ctx_t* ctx)
{
    const char* member_ids_str;
    u32* user_ids;
    size_t rows;

    if (status == PGRES_TUPLES_OK)
    {
        if (ctx->flags == DB_CTX_NO_JSON)
        {
            rows = PQntuples(res);
            user_ids = calloc(rows, sizeof(u32));

            for (size_t i = 0; i < rows; i++)
            {
                char* endptr;
                const char* user_id_str = PQgetvalue(res, i, 0);
                u32 user_id = strtoul(user_id_str, &endptr, 10);
                user_ids[i] = user_id;
            }
            ctx->data = user_ids;
            ctx->data_size = rows;
        }
        else
        {
            member_ids_str = PQgetvalue(res, 0, 0);
            if (member_ids_str == NULL)
                member_ids_str = "[]";
            ctx->param.member_ids.ids_json = member_ids_str;
        }
        ctx->ret = DB_ASYNC_OK;
    }
    else
    {
        ctx->ret = DB_ASYNC_ERROR;
        error("get_group_member_ids: %s\n",
              PQresultErrorMessage(res));
    }
}

bool 
db_async_get_group_member_ids(server_db_t* db, u32 group_id, dbcmd_ctx_t* ctx)
{
    const char* sql;
    if (ctx->flags == DB_CTX_NO_JSON)
        sql = "SELECT user_id FROM GroupMembers WHERE group_id = $1::int;";
    else
        sql = "SELECT json_agg(user_id) FROM GroupMembers WHERE group_id = $1::int;";

    i32 ret;
    char group_id_str[DB_INTSTR_MAX];
    const char* vals[1] = {
        group_id_str
    };
    const i32 lens[1] = {
        snprintf(group_id_str, DB_INTSTR_MAX, "%u", group_id)
    };
    const i32 formats[1] = {0};
    ctx->exec_res = db_get_group_member_ids_result;
    ret = db_async_params(db, sql, 1, vals, lens, formats, ctx);

    return ret == 1;
}

static void
do_get_group_msgs(UNUSED eworker_t* ew, PGresult* res, ExecStatusType status, dbcmd_ctx_t* ctx)
{
    const char* msgs_json_str;

    if (status == PGRES_TUPLES_OK)
    {
        msgs_json_str = PQgetvalue(res, 0, 0);
        if (msgs_json_str == NULL)
            msgs_json_str = "[]";

        ctx->param.group_msgs.msgs_json = msgs_json_str;
        ctx->ret = DB_ASYNC_OK;
    }
    else
    {
        error("get_group_msgs not PGRES_TUPLES_OK: %s\n",
              PQresultErrorMessage(res));
        ctx->ret = DB_ASYNC_ERROR;
    }
}

bool 
db_async_get_group_msgs(server_db_t* db, u32 group_id, u32 limit, u32 offset, dbcmd_ctx_t* ctx)
{
    i32 ret;
    char group_id_str[DB_INTSTR_MAX];
    char limit_str[DB_INTSTR_MAX];
    char offset_str[DB_INTSTR_MAX];
    const char* vals[3] = {
        group_id_str,
        limit_str,
        offset_str
    };
    const i32 lens[3] = {
        snprintf(group_id_str, DB_INTSTR_MAX, "%u", group_id),
        snprintf(limit_str, DB_INTSTR_MAX, "%u", limit),
        snprintf(offset_str, DB_INTSTR_MAX, "%u", offset)
    };
    const i32 formats[3] = {0};

    ctx->exec_res = do_get_group_msgs;
    ret = db_async_params(db, db->cmd->select_group_msgs_json, 3, vals, lens, formats, ctx);

    return ret == 1;
}

static void 
insert_group_msg_result(UNUSED eworker_t* ew, PGresult* res, ExecStatusType status, dbcmd_ctx_t* ctx)
{
    const char* msg_id_str;
    const char* timestamp;
    char* endptr;
    dbmsg_t* msg;

    if (status == PGRES_TUPLES_OK)
    {
        msg = ctx->data;
        msg_id_str = PQgetvalue(res, 0, 0);
        msg->msg_id = strtoul(msg_id_str, &endptr, 10);

        timestamp = PQgetvalue(res, 0, 1);
        strncpy(msg->timestamp, timestamp, DB_TIMESTAMP_MAX);
        ctx->ret = DB_ASYNC_OK;
    }
    else
    {
        error("insert_group_msg not tuples ok: %s\n",
              PQresultErrorMessage(res));
        ctx->ret = DB_ASYNC_ERROR;
    }
}

bool 
db_async_insert_group_msg(server_db_t* db, dbmsg_t* msg, dbcmd_ctx_t* ctx)
{
    i32 ret;
    char user_id_str[DB_INTSTR_MAX];
    char group_id_str[DB_INTSTR_MAX];
    if (!msg->attachments)
        msg->attachments = "[]";

    const char* vals[4] = {
        user_id_str,
        group_id_str,
        msg->content,
        msg->attachments
    };
    const i32 lens[4] = {
        snprintf(user_id_str, DB_INTSTR_MAX, "%u", msg->user_id),
        snprintf(group_id_str, DB_INTSTR_MAX, "%u", msg->group_id),
        strnlen(msg->content, DB_MESSAGE_MAX),
        strlen(msg->attachments)
    };
    const i32 formats[4] = {0};

    ctx->exec_res = insert_group_msg_result;
    ctx->data = msg;
    ret = db_async_params(db, db->cmd->insert_msg, 4, vals, lens, formats, ctx);
    return ret == 1;
}

static void
get_public_groups_result(UNUSED eworker_t* ew,
                         PGresult* res, ExecStatusType status, dbcmd_ctx_t* ctx)
{
    const char* groups_array_json;

    if (status == PGRES_TUPLES_OK)
    {
        groups_array_json = PQgetvalue(res, 0, 0);
        if (groups_array_json == NULL)
            groups_array_json = "[]";
        ctx->param.str = groups_array_json;
        ctx->ret = DB_ASYNC_OK;
    }
    else
    {
        error("Async get public groups: %s\n",
              PQresultErrorMessage(res));
        ctx->ret = DB_ASYNC_ERROR;
    }
}

bool 
db_async_get_public_groups(server_db_t* db, u32 user_id, dbcmd_ctx_t* ctx)
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
    ctx->exec_res = get_public_groups_result;
    ret = db_async_params(db, db->cmd->select_pub_group, 1, vals, lens, formats, ctx);
    return ret == 1;
}
