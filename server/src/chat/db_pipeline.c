#include "chat/db_pipeline.h"
#include "chat/db.h"
#include "chat/db_def.h"
#include <libpq-fe.h>
#include "json_object.h"
#include "server_eworker.h"
#include "server_websocket.h"

static const char* const pgres_status_str[] = {
	"PGRES_EMPTY_QUERY",
	"PGRES_COMMAND_OK",
	"PGRES_TUPLES_OK",
	"PGRES_COPY_OUT",
	"PGRES_COPY_IN",
	"PGRES_BAD_RESPONSE",
	"PGRES_NONFATAL_ERROR",
	"PGRES_FATAL_ERROR",
	"PGRES_COPY_BOTH",
	"PGRES_SINGLE_TUPLE",
	"PGRES_PIPELINE_SYNC",
	"PGRES_PIPELINE_ABORTE"
};

i32 
db_async_params(server_db_t* db, 
                const char* query,
                size_t n,
                const char* const vals[], 
                const i32* lens, 
                const i32* formats, 
                const dbcmd_ctx_t* cmd)
{
    i32 ret;
    if (!cmd)
        return 0;

    if ((ret = PQsendQueryParams(db->conn, query, n, NULL, vals, lens, formats, 0)) != 1)
    {
        error("Async query send: %s\n",
              PQerrorMessage(db->conn));
        goto err;
    }
    debug("Used SQL (curr: %p): %s\n", 
          db->ctx.head, query);
    if (db->queue.count == 0)
    {
        PQpipelineSync(db->conn);
    }
    else
    {
        PQsendFlushRequest(db->conn);
    }
    db_pipeline_enqueue_current(db, cmd);
err:
    return ret;
}

i32 
db_async_exec(server_db_t* db, const char* query,
              const dbcmd_ctx_t* cmd)
{
    return db_async_params(db, query, 0, NULL, NULL, NULL, cmd);
}

static void
db_exec_cmd(eworker_t* ew, dbcmd_ctx_t* cmd)
{
    const char* errmsg;
    json_object* resp;

    errmsg = cmd->exec(ew, cmd);
    if (errmsg && cmd->client)
    {
        resp = json_object_new_object();
        json_object_object_add(resp, "cmd",
                               json_object_new_string("error"));
        json_object_object_add(resp, "error_msg",
                               json_object_new_string(errmsg));
        ws_json_send(cmd->client, resp);
        json_object_put(resp);
    }
    db_pipeline_current_done(&ew->db);
}

static void
db_cmd_free(dbcmd_ctx_t* cmd)
{
    if (cmd == NULL)
        return;

    dbcmd_ctx_t* next;

    while (cmd)
    {
        next = cmd->next;
        free(cmd->data);
        free(cmd);
        cmd = next;
    }
}

void 
db_process_results(eworker_t* ew)
{
    PGresult* res;
    size_t count = 0;
    server_db_t* db = &ew->db;
    ExecStatusType status;
    dbcmd_ctx_t* ctx_peek;
    dbcmd_ctx_t* cmd_next;
    dbcmd_ctx_t cmd;

    while ((res = PQgetResult(db->conn)))
    {
        status = PQresultStatus(res);
        debug("> %zu: %s\n", count, pgres_status_str[status]);
        if (status == PGRES_PIPELINE_SYNC)
            goto clear;
        ctx_peek = db_pipeline_peek(db);
        if (!ctx_peek)
        {
            error("> %zu: Nothing in pipeline!\n", count);
            goto clear;
        }
        while (ctx_peek->next && ctx_peek->ret != DB_ASYNC_BUSY)
            if (ctx_peek->next)
                ctx_peek = ctx_peek->next;
        ctx_peek->exec_res(ew, res, status, ctx_peek);
        if (ctx_peek->next == NULL)
        {
            db_pipeline_dequeue(db, &cmd);
            db_exec_cmd(ew, &cmd);
            cmd_next = cmd.next;
            while (cmd_next)
            {
                if (cmd_next->exec != cmd.exec)
                {
                    db_exec_cmd(ew, cmd_next);
                    free(cmd_next->data);
                }
                cmd_next = cmd_next->next;
            }
            if (cmd.next)
                if (cmd.data != cmd.next->data)
                    free(cmd.data);
            db_cmd_free(cmd.next);
        }
    clear:
        count++;
        PQclear(res);
    }
}

i32
db_pipeline_enqueue(server_db_t* db, const dbcmd_ctx_t* cmd)
{
    plq_t* q = &db->queue;
    if (q->write->client != NULL)
        return -1;
    memcpy(q->write, cmd, sizeof(dbcmd_ctx_t));
    if ((q->write++ >= q->end))
        q->write = q->begin;
    q->count++;
    return 0;
}

i32
db_pipeline_enqueue_current(server_db_t* db, const dbcmd_ctx_t* cmd)
{
    if (!cmd)
    {
        warn("enqueue_current: cmd is null!\n");
        return -1;
    }

    dbcmd_ctx_t* next_cmd = malloc(sizeof(dbcmd_ctx_t));
    memcpy(next_cmd, cmd, sizeof(dbcmd_ctx_t));
    next_cmd->next = NULL;

    if (db->ctx.head == NULL)
    {
        next_cmd->client = db->ctx.client;
        db->ctx.head = next_cmd;
        db->ctx.tail = next_cmd;
        return 0;
    }
    db->ctx.tail->next = next_cmd;
    db->ctx.tail = next_cmd;
    return 0;
}

dbcmd_ctx_t* 
db_pipeline_peek(const server_db_t* db)
{
    const plq_t* q = &db->queue;
    if (q->read->exec == NULL)
        return NULL;
    return q->read;
}

i32
db_pipeline_dequeue(server_db_t* db, dbcmd_ctx_t* cmd)
{
    plq_t* q = &db->queue;
    if (q->read->exec == NULL)
        return 0;
    memcpy(cmd, q->read, sizeof(dbcmd_ctx_t));
    memset(q->read, 0, sizeof(dbcmd_ctx_t));
    if ((q->read++ >= q->end))
        q->read = q->begin;
    q->count--;
    return 1;
}

void
db_pipeline_reset_current(server_db_t* db)
{
    memset(&db->ctx, 0, sizeof(dbctx_t));
}

void
db_pipeline_current_done(server_db_t* db)
{
    debug("db->ctx.head: %p\n", db->ctx.head);
    if (db->ctx.head == NULL)
        return;

    db_pipeline_enqueue(db, db->ctx.head);
    free(db->ctx.head);
    db_pipeline_reset_current(db);
}

void 
db_pipeline_set_ctx(server_db_t* db, client_t* client)
{
    db->ctx.client = client;
}
