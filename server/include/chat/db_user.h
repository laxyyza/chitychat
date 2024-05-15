#ifndef _SERVER_DB_USER_H_
#define _SERVER_DB_USER_H_

#include "chat/db_def.h"

typedef struct dbuser dbuser_t;

bool db_async_get_user(server_db_t* db, u32 user_id, dbcmd_ctx_t* ctx);
bool db_async_get_user_username(server_db_t* db, const char* username, dbcmd_ctx_t* ctx);
bool db_async_get_user_array(server_db_t* db, const char* json_array, dbcmd_ctx_t* ctx);
bool db_async_insert_user(server_db_t* db, const dbuser_t* user, dbcmd_ctx_t* ctx);
bool db_async_get_connected_users(server_db_t* db, u32 user_id, dbcmd_ctx_t* ctx);

#endif // _SERVER_DB_USER_H_
