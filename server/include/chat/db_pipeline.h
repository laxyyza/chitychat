#ifndef _SERVER_DB_PIPELINE_H_
#define _SERVER_DB_PIPELINE_H_

#include "chat/db_def.h"
#include "chat/user.h"

/* Async operations */
i32 db_async_params(server_db_t* db, 
                    const char* query, 
                    size_t n, 
                    const char* const vals[], 
                    const i32* lens, 
                    const i32* formats,
                    const dbcmd_ctx_t* cmd);
i32 db_async_exec(server_db_t* db, const char* query,
                  const dbcmd_ctx_t* cmd);

void db_process_results(eworker_t* ew);

/* Pipeline */
i32 db_pipeline_enqueue(server_db_t* db, const dbcmd_ctx_t* cmd);            /* Enqueue to pipeline */
i32 db_pipeline_enqueue_current(server_db_t* db, const dbcmd_ctx_t* cmd);    /* Enqueue to current cmd */

dbcmd_ctx_t* db_pipeline_peek(const server_db_t* db); /* Peek */
i32 db_pipeline_dequeue(server_db_t* db, dbcmd_ctx_t* cmd); /* Dequeue Pipeline */

void db_pipeline_reset_current(server_db_t* db);    /* Set current to null */
void db_pipeline_current_done(server_db_t* db);     /* Enqueue current cmd to pipeline and reset current */

/* Groups */
i32 db_async_get_group(server_db_t* db, u32 group_id, dbexec_t callback);

#endif // _SERVER_DB_PIPELINE_H_
