#ifndef _SERVER_DB_
#define _SERVER_DB_

#include "common.h"
#include "server_crypt.h"

#include <sqlite3.h>

#define DB_USERNAME_MAX 50
#define DB_DISPLAYNAME_MAX 50
#define DB_MESSAGE_MAX  512
#define DB_TIMESTAMP_MAX 64
#define DB_BIO_MAX 256
#define DB_DESC_MAX 256
#define DB_PASSWORD_MAX 50
#define DB_PFP_HASH_MAX 64 + 1 // 64 bytes: 32-bytes (SHA256) + 32-bytes to string. + 1 for \0
#define DB_PFP_NAME_MAX NAME_MAX
#define DB_MIME_TYPE_LEN 32 

typedef struct 
{
    u64 user_id;
    char username[DB_USERNAME_MAX];
    char displayname[DB_DISPLAYNAME_MAX];
    char bio[DB_BIO_MAX];
    u8   hash[SERVER_HASH_SIZE];
    u8   salt[SERVER_SALT_SIZE];
    char created_at[DB_TIMESTAMP_MAX];
    char pfp_hash[DB_PFP_NAME_MAX];
    i32 flags;
    bool online;
} dbuser_t;

typedef struct 
{
    u64 group_id;
    u64 owner_id;
    char displayname[DB_DISPLAYNAME_MAX];
    char desc[DB_DESC_MAX];
    i32 flags;
} dbgroup_t;

typedef struct 
{
    u64 user_id;
    u64 group_id;
    char join_date[DB_TIMESTAMP_MAX];
    i32 flags;
} dbgroup_member_t;

typedef struct 
{
    u64 msg_id;
    u64 user_id;
    u64 group_id;
    char content[DB_MESSAGE_MAX];
    char timestamp[DB_TIMESTAMP_MAX];
    const char* attachments;
    i32 flags;
    i32 parent_msg_id;
} dbmsg_t;

typedef struct 
{
    char hash[DB_PFP_HASH_MAX];
    char name[DB_PFP_NAME_MAX];
    char mime_type[DB_MIME_TYPE_LEN];
    size_t size;
    i32 ref_count;
    i32 flags;
} dbuser_file_t;

typedef struct 
{
    sqlite3* db;

    char* schema;
    size_t schema_len;

    char* insert_user;
    size_t insert_user_len;
    char* select_user;
    size_t select_user_len;
    char* delete_user;
    size_t delete_user_len;

    char* insert_group;
    size_t insert_group_len;
    char* select_group;
    size_t select_group_len;
    char* delete_group;
    size_t delete_group_len;

    char* insert_groupmember;
    size_t insert_groupmember_len;
    char* select_groupmember;
    size_t select_groupmember_len;
    char* delete_groupmember;
    size_t delete_groupmember_len;

    char* insert_msg;
    size_t insert_msg_len;
    char* select_msg;
    size_t select_msg_len;
    char* delete_msg;
    size_t delete_msg_len;

    char* update_user;
    size_t update_user_len;

    char* insert_userfiles;
    size_t insert_userfiles_len;
} server_db_t;

bool        server_db_open(server_t* server);
void        server_db_close(server_t* server);

dbuser_t*   server_db_get_user_from_id(server_t* server, u64 user_id);
dbuser_t*   server_db_get_user_from_name(server_t* server, const char* username);
dbuser_t*   server_db_get_users_from_group(server_t* server, u64 group_id, u32* n);
bool        server_db_insert_user(server_t* server, dbuser_t* user);

bool        server_db_update_user(server_t* server, const char* new_username, 
const char* new_displayname, const char* new_pfp_name, const u64 user_id);

dbuser_t*   server_db_get_group_members(server_t* server, u64 group_id, u32* n);
bool        server_db_user_in_group(server_t* server, u64 group_id, u64 user_id);
bool        server_db_insert_group_member(server_t* server, u64 group_id, u64 user_id);

dbgroup_t*  server_db_get_group(server_t* server, u64 group_id);
dbgroup_t*  server_db_get_all_groups(server_t* server, u32* n);
dbgroup_t*  server_db_get_user_groups(server_t* server, u64 user_id, u32* n);
bool        server_db_insert_group(server_t* server, dbgroup_t* group);

dbmsg_t*    server_db_get_msg(server_t* server, u64 msg_id);
dbmsg_t*    server_db_get_msgs_from_group(server_t* server, u64 group_id, u32 limit, u32 offset, u32* n);
bool        server_db_insert_msg(server_t* server, dbmsg_t* msg);

dbuser_file_t*     server_db_select_userfile(server_t* server, const char* hash);
i32                 server_db_select_userfile_ref_count(server_t* server, const char* hash);
bool                server_db_insert_userfile(server_t* server, dbuser_file_t* file);
bool                server_db_delete_userfile(server_t* server, const char* hash);

#endif // _SERVER_DB_