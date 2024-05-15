#ifndef _SERVER_DB_PIPELINE_H_
#define _SERVER_DB_PIPELINE_H_

#include "chat/db_def.h"
#include "chat/user.h"

/**
 * Potential Issue:
 *  If a client disconnects immediately after making a request,
 *  it could result in a use-after-free scenario.
 *
 * Consideration:
 *  Implementing a reference counting mechanism for client requests in the pipeline
 *  could mitigate this issue. With this approach, any operations triggered by a disconnected
 *  client would be disregarded, and the client would only be freed once all asynchronous
 *  operations have concluded.
 */

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

/* Transactions */
i32 db_async_begin(server_db_t* db, dbcmd_ctx_t* ctx);
i32 db_async_commit(server_db_t* db);
i32 db_async_rollback(server_db_t* db);

/* Pipeline */
i32 db_pipeline_enqueue(server_db_t* db, const dbcmd_ctx_t* cmd);            /* Enqueue to pipeline */
i32 db_pipeline_enqueue_current(server_db_t* db, const dbcmd_ctx_t* cmd);    /* Enqueue to current cmd */

dbcmd_ctx_t* db_pipeline_peek(const server_db_t* db); /* Peek */
i32 db_pipeline_dequeue(server_db_t* db, dbcmd_ctx_t* cmd); /* Dequeue Pipeline */

void db_pipeline_reset_current(server_db_t* db);    /* Set current to null */
void db_pipeline_current_done(server_db_t* db);     /* Enqueue current cmd to pipeline and reset current */
void db_pipeline_set_ctx(server_db_t* db, client_t* client);

void db_process_results(eworker_t* ew);

#endif // _SERVER_DB_PIPELINE_H_
