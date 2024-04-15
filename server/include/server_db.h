#ifndef _SERVER_DB_
#define _SERVER_DB_

#include "common.h"
#include "server_crypt.h"

#include <libpq-fe.h>

#define DB_USERNAME_MAX     50
#define DB_DISPLAYNAME_MAX  50
#define DB_MESSAGE_MAX      512
#define DB_TIMESTAMP_MAX    64
#define DB_BIO_MAX          256
#define DB_DESC_MAX         256
#define DB_PASSWORD_MAX     50
#define DB_PFP_HASH_MAX     SERVER_HASH256_STR_SIZE
#define DB_PFP_NAME_MAX     NAME_MAX
#define DB_MIME_TYPE_LEN    32 
#define DB_GROUP_CODE_MAX   8

#define DB_CONNINTO_LEN     256

#define DB_GROUP_PUBLIC     0x01

#define DB_INTSTR_MAX       30

typedef struct 
{
    u32     user_id;
    char    username[DB_USERNAME_MAX];
    char    displayname[DB_DISPLAYNAME_MAX];
    char    bio[DB_BIO_MAX];
    u8      hash[SERVER_HASH_SIZE];
    u8      salt[SERVER_SALT_SIZE];
    char    created_at[DB_TIMESTAMP_MAX];
    char    pfp_hash[DB_PFP_NAME_MAX];
    i32     flags;
    bool    online;
} dbuser_t;

typedef struct 
{
    u32     group_id;
    u32     owner_id;
    char    displayname[DB_DISPLAYNAME_MAX];
    char    desc[DB_DESC_MAX];
    i32     flags;
    char    created_at[DB_TIMESTAMP_MAX];
} dbgroup_t;

typedef struct 
{
    u32     user_id;
    u32     group_id;
    char    join_date[DB_TIMESTAMP_MAX];
    i32     flags;
} dbgroup_member_t;

typedef struct 
{
    u32     msg_id;
    u32     user_id;
    u32     group_id;
    char    content[DB_MESSAGE_MAX];
    char    timestamp[DB_TIMESTAMP_MAX];
    char*           attachments;
    bool            attachments_inheap;
    json_object*    attachments_json;
    i32     flags;
    i32     parent_msg_id;
} dbmsg_t;

typedef struct 
{
    char    hash[DB_PFP_HASH_MAX];
    char    name[DB_PFP_NAME_MAX];
    char    mime_type[DB_MIME_TYPE_LEN];
    size_t  size;
    i32     ref_count;
    i32     flags;
} dbuser_file_t;

typedef struct 
{
    char    invite_code[DB_GROUP_CODE_MAX];
    u32     group_id;
    i32     uses;
    i32     max_uses;
} dbgroup_code_t;

typedef struct 
{
    char*   schema;
    size_t  schema_len;

    char*   insert_user;
    size_t  insert_user_len;
    char*   select_user;
    size_t  select_user_len;
    char*   delete_user;
    size_t  delete_user_len;

    char*   insert_group;
    size_t  insert_group_len;
    char*   select_group;
    size_t  select_group_len;
    char*   delete_group;
    size_t  delete_group_len;

    char*   select_groupmember;
    size_t  select_groupmember_len;
    char*   delete_groupmember;
    size_t  delete_groupmember_len;

    char*   insert_msg;
    size_t  insert_msg_len;
    char*   select_msg;
    size_t  select_msg_len;
    char*   delete_msg;
    size_t  delete_msg_len;

    char*   update_user;
    size_t  update_user_len;

    char*   insert_userfiles;
    size_t  insert_userfiles_len;

    char*   insert_groupmember_code;
    size_t  insert_groupmember_code_len;
} server_db_commands_t;

typedef struct 
{
    PGconn*                     conn;
    const server_db_commands_t* cmd;
} server_db_t;

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

dbuser_t*   server_db_get_group_members(server_db_t* db, u32 group_id, u32* n);
bool        server_db_user_in_group(server_db_t* db, u32 group_id, u32 user_id);
bool        server_db_insert_group_member(server_db_t* db, u32 group_id, u32 user_id);
dbgroup_t*  server_db_insert_group_member_code(server_db_t* db, 
                                               const char* invite_code, u32 user_id);
bool        server_db_delete_group_code(server_db_t* db, const char* invite_code);

dbgroup_t*  server_db_get_group(server_db_t* db, u32 group_id);
dbgroup_t*  server_db_get_group_from_invite(server_db_t* db, const char* invite_code);
dbgroup_t*  server_db_get_all_groups(server_db_t* db, u32* n);
dbgroup_t*  server_db_get_user_groups(server_db_t* db, u32 user_id, u32* n);
bool        server_db_insert_group(server_db_t* db, dbgroup_t* group);

dbmsg_t*    server_db_get_msg(server_db_t* db, u32 msg_id);
dbmsg_t*    server_db_get_msgs_from_group(server_db_t* db, u32 group_id, 
                                          u32 limit, u32 offset, u32* n);
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
