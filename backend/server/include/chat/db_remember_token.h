#ifndef _SERVER_CHAT_DB_REMEMBER_TOKEN_H_
#define _SERVER_CHAT_DB_REMEMBER_TOKEN_H_

#include "chat/db_def.h"
#include "server_auth_token.h"

bool db_async_insert_remember_token(server_db_t* db, remember_token_t* rt, dbcmd_ctx_t* ctx);
bool db_async_select_remember_token(server_db_t* db, const char* token, dbcmd_ctx_t* ctx);
bool db_async_delete_remember_token(server_db_t* db, const char* token, dbcmd_ctx_t* ctx);

#endif // _SERVER_CHAT_DB_REMEMBER_TOKEN_H_
