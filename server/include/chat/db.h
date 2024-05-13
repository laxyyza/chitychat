#ifndef _SERVER_DB_
#define _SERVER_DB_

#include "common.h"
#include "chat/db_def.h"
#include "chat/user.h"
#include "chat/group.h"
#include "chat/user_file.h"
#include "chat/user_login.h"

#define DB_DEFAULT      0x00
#define DB_PIPELINE     0x01
#define DB_NONBLOCK     0x02

#define DB_ASYNC_BUSY   0
#define DB_ASYNC_OK     1
#define DB_ASYNC_ERROR -1

#define DB_CTX_NO_JSON  0x01

typedef struct 
{
    u32 group_id;
    const char* ids_json;
} member_ids_param_t;

typedef struct 
{
    u32 group_id; 
    const char* msgs_json;
} group_msgs_param_t;

union cmd_param 
{
    user_login_param_t user_login;
    member_ids_param_t member_ids;
    group_msgs_param_t group_msgs;
    session_t*  session;
    json_object* json;
    const char* str;
    void*       ptr;
    u32         group_id;
    u32         user_id;
};

typedef struct dbcmd_ctx
{
    i32       ret;
    i32       flags;
    client_t* client;
    void*     data;
    size_t    data_size;
    dbexec_t     exec;
    dbexec_res_t exec_res;
    union cmd_param param;
    struct dbcmd_ctx* next;
} dbcmd_ctx_t;

typedef struct 
{
    dbcmd_ctx_t* begin;
    dbcmd_ctx_t* end;
    dbcmd_ctx_t* read;
    dbcmd_ctx_t* write;
    size_t size;
    size_t count;
} pipeline_queue_t, plq_t;

typedef struct 
{
    i32           ret;
    client_t*     client;
    dbcmd_ctx_t*  head;
    dbcmd_ctx_t*  tail;
} dbctx_t;

typedef struct server_db
{
    i32     fd;
    i32     flags;
    PGconn* conn;
    plq_t   queue;
    dbctx_t ctx;
    const server_db_commands_t* cmd;
} server_db_t;

bool        server_init_db(server_t* server);
bool        server_db_open(server_db_t* db, const char* dbname, i32 flags);
void        server_db_free(server_t* server);
void        server_db_close(server_db_t* db);

PGresult*   server_db_params(server_db_t* db, 
                             const char* cmd, 
                             size_t n,
                             const char* const vals[],
                             const i32* lens,
                             const i32* formats);
PGresult*   server_db_exec(server_db_t* db, const char* cmd);

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
dbgroup_t*  server_db_get_group_from_invite(server_db_t* db, const char* invite_code);
dbgroup_t*  server_db_get_public_groups(server_db_t* db, u32 user_id, u32* n);
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

/* Result to structure */
void db_row_to_user(dbuser_t* user, PGresult* res, i32 row);
void db_row_to_group(dbgroup_t* group, PGresult* res, i32 row);

/* Frees */
void dbmsg_free(dbmsg_t* msg);

#endif // _SERVER_DB_
