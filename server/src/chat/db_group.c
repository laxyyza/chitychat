#include "chat/db_pipeline.h"
#include "chat/db_group.h"
#include "chat/db_def.h"
#include "chat/db.h"
#include "chat/group.h"
#include <libpq-fe.h>
#include <stdio.h>

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
        if (ctx->flags & DB_CTX_NO_JSON)
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
    if (ctx->flags & DB_CTX_NO_JSON)
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
db_delete_msg_result(UNUSED eworker_t* ew,
                     PGresult* res, ExecStatusType status, dbcmd_ctx_t* ctx)
{
    const char* group_id_str;
    const char* attachments_str;

    if (status == PGRES_TUPLES_OK)
    {
        group_id_str = PQgetvalue(res, 0, 0);
        ctx->param.del_msg.group_id = strtoul(group_id_str, NULL, 0);

        attachments_str = PQgetvalue(res, 0, 1);
        ctx->param.del_msg.attachments_json = attachments_str;

        ctx->ret = DB_ASYNC_OK;
        return;
    }
    else if (status == PGRES_FATAL_ERROR)
        error("Delete group message: %s\n",
              PQresultErrorMessage(res));
    ctx->ret = DB_ASYNC_ERROR;
}

bool 
db_async_delete_msg(server_db_t* db, u32 msg_id, u32 user_id, dbcmd_ctx_t* ctx)
{
    i32 ret;
    char msg_id_str[DB_INTSTR_MAX];
    char user_id_str[DB_INTSTR_MAX];
    const char* vals[2] = {
        msg_id_str,
        user_id_str
    };
    const i32 lens[2] = {
        snprintf(msg_id_str, DB_INTSTR_MAX, "%u", msg_id),
        snprintf(user_id_str, DB_INTSTR_MAX, "%u", user_id),
    };
    const i32 formats[2] = {0, 0};
    ctx->exec_res = db_delete_msg_result;
    ret = db_async_params(db, db->cmd->delete_msg, 2, vals, lens, formats, ctx);
    return ret;
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

static void
join_pub_group_result(UNUSED eworker_t* ew, 
                      PGresult* res, ExecStatusType status, dbcmd_ctx_t* ctx)
{
    if (status == PGRES_COMMAND_OK)
        ctx->ret = DB_ASYNC_OK;
    else
    {
        error("Failed to join pub group: %s\n",
              PQresultErrorMessage(res));
        ctx->ret = DB_ASYNC_ERROR;
    }
}

bool 
db_async_user_join_pub_group(server_db_t* db, u32 user_id, u32 group_id, dbcmd_ctx_t* ctx)
{
    i32 ret;
    char user_id_str[DB_INTSTR_MAX];
    char group_id_str[DB_INTSTR_MAX];
    const char* vals[2] = {
        user_id_str,
        group_id_str
    };
    const i32 lens[2] = {
        snprintf(user_id_str, DB_INTSTR_MAX, "%u", user_id),
        snprintf(group_id_str, DB_INTSTR_MAX, "%u", group_id),
    };
    const i32 formats[2] = {0};
    ctx->exec_res = join_pub_group_result;
    ret = db_async_params(db, db->cmd->insert_pub_groupmember, 2, vals, lens, formats, ctx);
    return ret == 1;
}

static void
create_group_result(UNUSED eworker_t* ew, 
                    PGresult* res, ExecStatusType status, dbcmd_ctx_t* ctx)
{
    const char* group_id_str;
    dbgroup_t* group = ctx->data;

    if (status == PGRES_TUPLES_OK)
    {
        group_id_str = PQgetvalue(res, 0, 0);
        group->group_id = strtoul(group_id_str, NULL, 0);
        ctx->ret = DB_ASYNC_OK;
    }
    else
    {
        error("create group: %s\n",
              PQresultErrorMessage(res));
        ctx->ret = DB_ASYNC_ERROR;
    }
}

bool 
db_async_create_group(server_db_t* db, dbgroup_t* group, dbcmd_ctx_t* ctx)
{
    i32 ret;
    char owner_id_str[DB_INTSTR_MAX];
    const char* public_str = (group->public) ? "true" : "false";
    const char* vals[3] = {
        group->displayname,
        owner_id_str,
        public_str
    };
    const i32 lens[3] = {
        strnlen(group->displayname, DB_DISPLAYNAME_MAX),
        snprintf(owner_id_str, DB_INTSTR_MAX, "%u", group->owner_id),
        strlen(public_str)
    };
    const i32 formats[3] = {0};
    ctx->exec_res = create_group_result;
    ctx->data = group;
    ret = db_async_params(db, db->cmd->insert_group, 3, vals, lens, formats, ctx);
    return ret;
}

static void
create_group_code_result(UNUSED eworker_t* ew,
                         PGresult* res, ExecStatusType status, dbcmd_ctx_t* ctx)
{
    i32 rows;
    const char* invite_code;
    dbgroup_code_t* group_code = ctx->data;

    if (status == PGRES_TUPLES_OK)
    {
        rows = PQntuples(res);
        if (rows == 0)
            goto err;
        invite_code = PQgetvalue(res, 0, 0);
        strncpy(group_code->invite_code, invite_code, DB_GROUP_CODE_MAX);
        ctx->ret = DB_ASYNC_OK;
        return;
    }
    else if (status == PGRES_FATAL_ERROR)
        error("Create Group Code: %s\n",
              PQresultErrorMessage(res));
err:
    ctx->ret = DB_ASYNC_ERROR;
}

bool 
db_async_create_group_code(server_db_t* db, 
                           dbgroup_code_t* group_code,
                           u32 user_id, 
                           dbcmd_ctx_t* ctx)
{
    i32 ret;
    char group_id_str[DB_INTSTR_MAX];
    char max_uses_str[DB_INTSTR_MAX];
    char user_id_str[DB_INTSTR_MAX];
    const char* vals[3] = {
        group_id_str,
        max_uses_str,
        user_id_str
    };
    const i32 lens[3] = {
        snprintf(group_id_str, DB_INTSTR_MAX, "%u", group_code->group_id),
        snprintf(max_uses_str, DB_INTSTR_MAX, "%d", group_code->max_uses),
        snprintf(user_id_str, DB_INTSTR_MAX, "%u", user_id),
    };
    const i32 formats[3] = {0, 0, 0};
    ctx->exec_res = create_group_code_result;
    ctx->data = group_code;
    ret = db_async_params(db, db->cmd->create_group_code, 3, vals, lens, formats, ctx);
    return ret;
}

static void
get_group_codes_result(UNUSED eworker_t* ew,
                       PGresult* res, ExecStatusType status, dbcmd_ctx_t* ctx)
{
    const char* group_codes_array_json;

    if (status == PGRES_TUPLES_OK)
    {
        group_codes_array_json = PQgetvalue(res, 0, 0);
        if (group_codes_array_json == NULL)
            group_codes_array_json = "[]";
        ctx->param.group_codes.array_json = group_codes_array_json;
        ctx->ret = DB_ASYNC_OK;
    }
    else
    {
        error("Get Group Codes: %s\n", 
              PQresultErrorMessage(res));
        ctx->ret = DB_ASYNC_ERROR;
    }
}

bool 
db_async_get_group_codes(server_db_t* db, u32 group_id, u32 user_id, dbcmd_ctx_t* ctx)
{
    i32 ret;
    char group_id_str[DB_INTSTR_MAX];
    char user_id_str[DB_INTSTR_MAX];
    const char* vals[2] = {
        group_id_str,
        user_id_str
    };
    const i32 lens[2] = {
        snprintf(group_id_str, DB_INTSTR_MAX, "%u", group_id),
        snprintf(user_id_str, DB_INTSTR_MAX, "%u", user_id),
    };
    const i32 formats[2] = {0, 0};
    ctx->exec_res = get_group_codes_result;
    ctx->param.group_codes.group_id = group_id;
    ret = db_async_params(db, db->cmd->get_group_code, 2, vals, lens, formats, ctx);
    return ret;
}

static void
user_join_group_code_result(UNUSED eworker_t* ew,
                            PGresult* res, ExecStatusType status, dbcmd_ctx_t* ctx)
{
    const char* group_id_str;
    u32 group_id;

    if (status == PGRES_TUPLES_OK)
    {
        if ((group_id_str = PQgetvalue(res, 0, 0)) == NULL)
            goto err;
        group_id = strtoul(group_id_str, NULL, 0);
        ctx->param.group_id = group_id;
        ctx->ret = DB_ASYNC_OK;
        return;
    }
    else if (status == PGRES_FATAL_ERROR)
        error("User join group via code: %s\n",
              PQresultErrorMessage(res));
err:
    ctx->ret = DB_ASYNC_ERROR;
}

bool 
db_async_user_join_group_code(server_db_t* db, 
                              const char* code, u32 user_id, dbcmd_ctx_t* ctx)
{
    i32 ret;
    char user_id_str[DB_INTSTR_MAX];
    const char* vals[2] = {
        user_id_str,
        code
    };
    const i32 lens[2] = {
        snprintf(user_id_str, DB_INTSTR_MAX, "%u", user_id),
        strnlen(code, DB_GROUP_CODE_MAX)
    };
    const i32 formats[2] = {0, 0};
    ctx->exec_res = user_join_group_code_result;
    ret = db_async_params(db, db->cmd->insert_groupmember_code, 2, vals, lens, formats, ctx);
    return ret;
}

static void
db_delete_group_code_result(UNUSED eworker_t* ew,
                            PGresult* res, ExecStatusType status, dbcmd_ctx_t* ctx)
{
    if (status == PGRES_COMMAND_OK)
    {
        ctx->ret = DB_ASYNC_OK;
        return;
    }
    else if (status == PGRES_FATAL_ERROR)
        error("Delete group code: %s\n",
              PQresultErrorMessage(res));
    ctx->ret = DB_ASYNC_ERROR;
}

bool 
db_async_delete_group_code(server_db_t* db, 
                           const char* invite_code, u32 user_id, dbcmd_ctx_t* ctx)
{
    i32 ret;
    char user_id_str[DB_INTSTR_MAX];
    const char* vals[2] = {
        invite_code,
        user_id_str
    };
    const i32 lens[2] = {
        strnlen(invite_code, DB_GROUP_CODE_MAX),
        snprintf(user_id_str, DB_INTSTR_MAX, "%u", user_id)
    };
    const i32 formats[2] = {0, 0};
    ctx->exec_res = db_delete_group_code_result;
    ret = db_async_params(db, db->cmd->delete_group_code, 2, vals, lens, formats, ctx);
    return ret;
}

static void
del_group_result(UNUSED eworker_t* ew,
                 PGresult* res, ExecStatusType status, dbcmd_ctx_t* ctx)
{
    if (status == PGRES_COMMAND_OK)
        ctx->ret = DB_ASYNC_OK;
    else
    {
        error("Delete group failed: %s\n",
              PQresultErrorMessage(res));
        ctx->ret = DB_ASYNC_ERROR;
    }
}

static void
get_all_attachs_result(UNUSED eworker_t* ew, 
                       PGresult* res, ExecStatusType status, dbcmd_ctx_t* ctx)
{
    i32 rows;
    json_object** attachs_array;

    if (status == PGRES_TUPLES_OK)
    {
        ctx->ret = DB_ASYNC_OK;
        rows = PQntuples(res);
        if (rows == 0)
            return;

        attachs_array = calloc(rows, sizeof(void*));
        for (i32 i = 0; i < rows; i++)
        {
            json_object** json_obj = attachs_array + i;
            const char* attach = PQgetvalue(res, i, 0);
            *json_obj = json_tokener_parse(attach);
        }
        ctx->data = attachs_array;
        ctx->data_size = rows;
    }
    else
    {
        error("Get all attachs: %s\n",
              PQresultErrorMessage(res));
        ctx->ret = DB_ASYNC_ERROR;
    }
}

/*
 * Quite a heavy operation:
 *      1. Select all it's messages with attachments (for further deletion and unlinking from filesystem)
 *      2. Select all it's member IDs (to broadcast to them that the group was deleted)
 *      3. Delete all it's messages
 *      4. Delete all it's group members
 *      5. Delete all it's group codes
 *      6. Delete Group
 */
bool
db_async_delete_group(server_db_t* db, u32 group_id, dbcmd_ctx_t* ctx)
{
    const char* get_all_attachs     = "SELECT attachments FROM Messages WHERE group_id = $1::int AND json_array_length(attachments) > 0;";
    const char* del_group_msg       = "DELETE FROM Messages WHERE group_id = $1::int;";
    const char* del_group_members   = "DELETE FROM GroupMembers WHERE group_id = $1::int;";
    const char* del_group_codes     = "DELETE FROM GroupCodes WHERE group_id = $1::int;";
    const char* del_group           = "DELETE FROM Groups WHERE group_id = $1::int;";
    i32 ret;
    char group_id_str[DB_INTSTR_MAX];
    const char* vals[1] = {
        group_id_str,
    };
    const i32 lens[1] = {
        snprintf(group_id_str, DB_INTSTR_MAX, "%u", group_id),
    };
    const i32 formats[1] = {0};

    /* Begin transaction */
    if (!(ret = db_async_begin(db, ctx)))
        return false;
    ctx->exec = NULL;

    /* Get all messages with atttachments */
    ctx->exec_res = get_all_attachs_result;
    if (!(ret = db_async_params(db, get_all_attachs, 1, vals, lens, formats, ctx)))
        goto rollback;

    /* Get all (former) member IDs */
    ctx->flags |= DB_CTX_NO_JSON;
    if (!(ret = db_async_get_group_member_ids(db, group_id, ctx)))
        goto rollback;
    ctx->exec_res = del_group_result;

    /* Delete all messages */
    if (!(ret = db_async_params(db, del_group_msg, 1, vals, lens, formats, ctx)))
        goto rollback;

    /* Delete all group members */
    if (!(ret = db_async_params(db, del_group_members, 1, vals, lens, formats, ctx)))
        goto rollback;
    
    /* Delete all group codes */
    if (!(ret = db_async_params(db, del_group_codes, 1, vals, lens, formats, ctx)))
        goto rollback;

    /* Delete group */
    if (!(ret = db_async_params(db, del_group, 1, vals, lens, formats, ctx)))
        goto rollback;

    ret = db_async_commit(db);
    return ret;
rollback:
    db_async_rollback(db);
    return false;
}

static void
get_group_owner_result(UNUSED eworker_t* ew, 
                       PGresult* res, ExecStatusType status, dbcmd_ctx_t* ctx)
{
    const char* owner_id_str;
    u32 owner_id;

    if (status == PGRES_TUPLES_OK)
    {
        if (PQntuples(res) == 0)
        {
            error("PQntuples() is 0\n");
            ctx->ret = DB_ASYNC_ERROR;
            return;
        }
        owner_id_str = PQgetvalue(res, 0, 0);
        owner_id = strtoul(owner_id_str, NULL, 0);
        printf("owner_id_str: %s\n", owner_id_str);
        ctx->param.group_owner.owner_id = owner_id;
        ctx->ret = DB_ASYNC_OK;
        return;
    }
    else if (status == PGRES_FATAL_ERROR)
        error("Get group owner: %s\n",
              PQresultErrorMessage(res));
    ctx->ret = DB_ASYNC_ERROR;
}

bool 
db_async_get_group_owner(server_db_t* db, u32 group_id, dbcmd_ctx_t* ctx)
{
    const char* sql = "SELECT owner_id FROM Groups WHERE group_id = $1::int;";
    i32 ret;
    char group_id_str[DB_INTSTR_MAX];
    const char* vals[1] = {
        group_id_str
    };
    const i32 lens[1] = {
        snprintf(group_id_str, DB_INTSTR_MAX, "%u", group_id)
    };
    const i32 formats[1] = {0};
    ctx->exec_res = get_group_owner_result;
    ret = db_async_params(db, sql, 1, vals, lens, formats, ctx);
    return ret;
}
