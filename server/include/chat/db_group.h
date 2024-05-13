#ifndef _SERVER_DB_GROUP_H_
#define _SERVER_DB_GROUP_H_

#include "chat/db_def.h"

typedef struct dbmsg dbmsg_t;

bool db_async_get_group(server_db_t* db, u32 group_id, dbcmd_ctx_t* ctx);
bool db_async_get_user_groups(server_db_t* db, u32 user_id, dbcmd_ctx_t* ctx);
bool db_async_get_group_member_ids(server_db_t* db, u32 group_id, dbcmd_ctx_t* ctx);
bool db_async_get_group_msgs(server_db_t* db, u32 group_id, u32 limit, u32 offset, dbcmd_ctx_t* ctx);

/*
 * `msg` param must be allocated on heap.
 * It will be freed if it was succesful.
 */
bool db_async_insert_group_msg(server_db_t* db, dbmsg_t* msg, dbcmd_ctx_t* ctx);

#endif // _SERVER_DB_GROUP_H_
