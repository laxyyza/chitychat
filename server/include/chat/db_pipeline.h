#ifndef _SERVER_DB_PIPELINE_H_
#define _SERVER_DB_PIPELINE_H_

#include "chat/db_def.h"

/* Async operations */
i32         server_db_async_params(server_db_t* db, 
                                   const char* query, 
                                   size_t n, 
                                   const char* const vals[], 
                                   const i32* lens, 
                                   const i32* formats);
i32         server_db_async_exec(server_db_t* db, const char* query);

/* Pipeline */
i32 db_pipeline_enqueue(server_db_t* db, const struct dbasync_cmd* cmd);            /* Enqueue to pipeline */
i32 db_pipeline_enqueue_current(server_db_t* db, const struct dbasync_cmd* cmd);    /* Enqueue to current cmd */

const struct dbasync_cmd* db_pipeline_peek(const server_db_t* db); /* Peek */
i32 db_pipeline_dequeue(server_db_t* db, struct dbasync_cmd* cmd); /* Dequeue Pipeline */

void db_pipeline_reset_current(server_db_t* db);    /* Set current to null */
void db_pipeline_current_done(server_db_t* db);     /* Enqueue current cmd to pipeline and reset current */

#endif // _SERVER_DB_PIPELINE_H_
