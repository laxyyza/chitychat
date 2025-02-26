#ifndef _DB_USER_SESSION_H_
#define _DB_USER_SESSION_H_

#include "chat/db_def.h"

typedef struct dbsession dbsession_t;

bool db_async_insert_session(server_db_t* db, const dbsession_t* session, dbcmd_ctx_t* ctx);
bool db_async_select_session(server_db_t* db, const dbsession_t* session, dbcmd_ctx_t* ctx);

#endif // _DB_USER_SESSION_H_