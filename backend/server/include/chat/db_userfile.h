#ifndef _SERVER_DB_USERFILE_H_
#define _SERVER_DB_USERFILE_H_

#include "chat/db_def.h"
#include "chat/db_pipeline.h"
#include "chat/user_file.h"

bool db_async_insert_userfile(server_db_t* db, dbuser_file_t* file, dbcmd_ctx_t* ctx);
bool db_async_delete_userfile(server_db_t* db, const char* hash, dbcmd_ctx_t* ctx);
bool db_async_userfile_refcount(server_db_t* db, const char* hash, dbcmd_ctx_t* ctx);

#endif // _SERVER_DB_USERFILE_H_
