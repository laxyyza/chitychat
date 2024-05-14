#ifndef _SERVER_DB_GROUP_H_
#define _SERVER_DB_GROUP_H_

#include "chat/db.h"
#include "chat/db_def.h"
#include "chat/group.h"

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

bool db_async_get_public_groups(server_db_t* db, u32 user_id, dbcmd_ctx_t* ctx);

bool db_async_user_join_pub_group(server_db_t* db, u32 user_id, u32 group_id, dbcmd_ctx_t* ctx);

bool db_async_create_group(server_db_t* db, dbgroup_t* group, dbcmd_ctx_t* ctx);

/* `group_code` must be allocated on the heap. */
bool db_async_create_group_code(server_db_t* db, 
                                dbgroup_code_t* group_code, 
                                u32 user_id, 
                                dbcmd_ctx_t* ctx);
bool db_async_get_group_codes(server_db_t* db, u32 group_id, u32 user_id, dbcmd_ctx_t* ctx);
bool db_async_user_join_group_code(server_db_t* db, 
                                   const char* code, u32 group_id, dbcmd_ctx_t* ctx);

#endif // _SERVER_DB_GROUP_H_
