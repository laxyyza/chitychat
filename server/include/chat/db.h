#ifndef _SERVER_DB_
#define _SERVER_DB_

#include "common.h"
#include "chat/db_def.h"
#include "chat/user.h"
#include "chat/group.h"
#include "chat/user_file.h"

bool        server_init_db(server_t* server);
bool        server_db_open(server_db_t* db, const char* dbname);
void        server_db_free(server_t* server);
void        server_db_close(server_db_t* db);

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

dbuser_t*   server_db_get_group_members(server_db_t* db, u32 group_id, u32* n);
bool        server_db_user_in_group(server_db_t* db, u32 group_id, u32 user_id);
bool        server_db_insert_group_member(server_db_t* db, u32 group_id, u32 user_id);
dbgroup_t*  server_db_insert_group_member_code(server_db_t* db, 
                                               const char* invite_code, u32 user_id);
bool        server_db_delete_group_code(server_db_t* db, const char* invite_code);
bool        server_db_delete_group(server_db_t* db, u32 group_id);

dbgroup_t*  server_db_get_group(server_db_t* db, u32 group_id);
dbgroup_t*  server_db_get_group_from_invite(server_db_t* db, const char* invite_code);
dbgroup_t*  server_db_get_all_groups(server_db_t* db, u32* n);
dbgroup_t*  server_db_get_user_groups(server_db_t* db, u32 user_id, u32* n);
bool        server_db_insert_group(server_db_t* db, dbgroup_t* group);

dbmsg_t*    server_db_get_msg(server_db_t* db, u32 msg_id);
dbmsg_t*    server_db_get_msgs_from_group(server_db_t* db, u32 group_id, 
                                          u32 limit, u32 offset, u32* n);
dbmsg_t*    server_db_get_msgs_only_attachs(server_db_t* db, u32 group_id, u32* n);

bool        server_db_insert_msg(server_db_t* db, dbmsg_t* msg);
bool        server_db_delete_msg(server_db_t* db, u32 msg_id);

dbgroup_code_t* server_db_get_group_code(server_db_t* db, const char* invite_code);
dbgroup_code_t* server_db_get_all_group_codes(server_db_t* db, u32 group_id, u32* n);
bool            server_db_insert_group_code(server_db_t* db, dbgroup_code_t* code);

dbuser_file_t*      server_db_select_userfile(server_db_t* db, const char* hash);
i32                 server_db_select_userfile_ref_count(server_db_t* db, const char* hash);
bool                server_db_insert_userfile(server_db_t* db, dbuser_file_t* file);
bool                server_db_delete_userfile(server_db_t* db, const char* hash);

void dbmsg_free(dbmsg_t* msg);

#endif // _SERVER_DB_
