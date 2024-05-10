#ifndef _SERVER_DB_
#define _SERVER_DB_

#include "common.h"
#include "chat/db_def.h"
#include "chat/user.h"
#include "chat/group.h"
#include "chat/user_file.h"

#define DB_DEFAULT      0x00
#define DB_PIPELINE     0x01
#define DB_NONBLOCK     0x02

bool        server_init_db(server_t* server);
bool        server_db_open(server_db_t* db, const char* dbname, i32 flags);
void        server_db_free(server_t* server);
void        server_db_close(server_db_t* db);

/* Async operations */
i32         server_db_async_params(server_db_t* db, const char* query, size_t n, const char* const vals[], const i32* lens, const i32* formats);
i32         server_db_async_exec(server_db_t* db, const char* query);

/* Pipeline */
i32 db_pipeline_enqueue(server_db_t* db, const struct dbasync_cmd* cmd);            /* Enqueue to pipeline */
i32 db_pipeline_enqueue_current(server_db_t* db, const struct dbasync_cmd* cmd);    /* Enqueue to current cmd */

const struct dbasync_cmd* db_pipeline_peek(const server_db_t* db); /* Peek */
i32 db_pipeline_dequeue(server_db_t* db, struct dbasync_cmd* cmd); /* Dequeue Pipeline */

void db_pipeline_reset_current(server_db_t* db);    /* Set current to null */
void db_pipeline_current_done(server_db_t* db);     /* Enqueue current cmd to pipeline and reset current */

/* Users */
dbuser_t*   server_db_get_user_from_id(server_db_t* db, u32 user_id);
dbuser_t*   server_db_get_user_from_name(server_db_t* db, const char* username);
dbuser_t*   server_db_get_users_from_group(server_db_t* db, u32 group_id, u32* n);
bool        server_db_insert_user(server_db_t* db, dbuser_t* user);
bool        server_db_update_user(server_db_t* db, 
                                  const char* new_username, 
                                  const char* new_displayname, 
                                  const char* new_pfp_name, 
                                  const u32 user_id);
dbuser_t*   server_db_get_connected_users(server_db_t* db, u32 user_id, u32* n);

/* Group Members */
dbuser_t*   server_db_get_group_members(server_db_t* db, u32 group_id, u32* n);
bool        server_db_user_in_group(server_db_t* db, u32 group_id, u32 user_id);
bool        server_db_insert_group_member(server_db_t* db, u32 group_id, u32 user_id);
dbgroup_t*  server_db_insert_group_member_code(server_db_t* db, 
                                               const char* invite_code, u32 user_id);
bool        server_db_delete_group_code(server_db_t* db, const char* invite_code);
bool        server_db_delete_group(server_db_t* db, u32 group_id);

/* Groups */
dbgroup_t*  server_db_get_group(server_db_t* db, u32 group_id);
dbgroup_t*  server_db_get_group_from_invite(server_db_t* db, const char* invite_code);
dbgroup_t*  server_db_get_public_groups(server_db_t* db, u32 user_id, u32* n);
dbgroup_t*  server_db_get_user_groups(server_db_t* db, u32 user_id, u32* n);
bool        server_db_insert_group(server_db_t* db, dbgroup_t* group);

/* Messages */
dbmsg_t*    server_db_get_msg(server_db_t* db, u32 msg_id);
dbmsg_t*    server_db_get_msgs_from_group(server_db_t* db, u32 group_id, 
                                          u32 limit, u32 offset, u32* n);
dbmsg_t*    server_db_get_msgs_only_attachs(server_db_t* db, u32 group_id, u32* n);
bool        server_db_insert_msg(server_db_t* db, dbmsg_t* msg);
bool        server_db_delete_msg(server_db_t* db, u32 msg_id);

/* Group Codes */
dbgroup_code_t* server_db_get_group_code(server_db_t* db, const char* invite_code);
dbgroup_code_t* server_db_get_all_group_codes(server_db_t* db, u32 group_id, u32* n);
bool            server_db_insert_group_code(server_db_t* db, dbgroup_code_t* code);

/* User files */
dbuser_file_t*      server_db_select_userfile(server_db_t* db, const char* hash);
i32                 server_db_select_userfile_ref_count(server_db_t* db, const char* hash);
bool                server_db_insert_userfile(server_db_t* db, dbuser_file_t* file);
bool                server_db_delete_userfile(server_db_t* db, const char* hash);

/* Frees */
void dbmsg_free(dbmsg_t* msg);

#endif // _SERVER_DB_
