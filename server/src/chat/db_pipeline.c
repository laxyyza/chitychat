#include "chat/db_pipeline.h"
#include "chat/db.h"

i32
db_pipeline_enqueue(server_db_t* db, const struct dbasync_cmd* cmd)
{
    plq_t* q = &db->queue;
    if (q->write->client != NULL)
        return -1;
    memcpy(q->write, cmd, sizeof(struct dbasync_cmd));
    if ((q->write++ >= q->end))
        q->write = q->begin;
    q->count++;
    return 0;
}

i32
db_pipeline_enqueue_current(server_db_t* db, const struct dbasync_cmd* cmd)
{
    if (!cmd || !cmd->client || !cmd->data)
        return -1;

    struct dbasync_cmd* next_cmd = malloc(sizeof(struct dbasync_cmd));
    memcpy(next_cmd, cmd, sizeof(struct dbasync_cmd));
    next_cmd->next = NULL;

    if (db->current.head == NULL)
    {
        db->current.head = next_cmd;
        db->current.tail = next_cmd;
        return 0;
    }
    db->current.tail->next = next_cmd;
    db->current.tail = next_cmd;
    return 0;
}

const struct dbasync_cmd* 
db_pipeline_peek(const server_db_t* db)
{
    const plq_t* q = &db->queue;
    if (q->read->client == NULL)
        return NULL;
    return q->read;
}

i32
db_pipeline_dequeue(server_db_t* db, struct dbasync_cmd* cmd)
{
    plq_t* q = &db->queue;
    if (q->read->client == NULL)
        return 0;
    memcpy(cmd, q->read, sizeof(struct dbasync_cmd));
    memset(q->read, 0, sizeof(struct dbasync_cmd));
    if ((q->read++ >= q->end))
        q->read = q->begin;
    q->count--;
    return 1;
}

void
db_pipeline_reset_current(server_db_t* db)
{
    db->current.head = NULL;
    db->current.tail = NULL;
}

void
db_pipeline_current_done(server_db_t* db)
{
    if (db->current.head == NULL)
        return;

    db_pipeline_enqueue(db, db->current.head);
    db_pipeline_reset_current(db);
}

