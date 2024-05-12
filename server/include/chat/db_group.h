#ifndef _SERVER_DB_GROUP_H_
#define _SERVER_DB_GROUP_H_

#include "chat/db_def.h"

bool db_async_get_group(server_db_t* db, u32 group_id, dbcmd_ctx_t* ctx);
bool db_async_get_user_groups(server_db_t* db, u32 user_id, dbcmd_ctx_t* ctx);
bool db_async_get_group_member_ids(server_db_t* db, u32 group_id, dbcmd_ctx_t* ctx);

#endif // _SERVER_DB_GROUP_H_
