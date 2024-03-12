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
#define DB_PFP_NAME_MAX NAME_MAX

typedef struct 
{
    u64 user_id;
    char username[DB_USERNAME_MAX];
    char displayname[DB_DISPLAYNAME_MAX];
    char bio[DB_BIO_MAX];
    u8   hash[SERVER_HASH_SIZE];
    u8   salt[SERVER_SALT_SIZE];
    char created_at[DB_TIMESTAMP_MAX];
    char pfp_name[DB_PFP_NAME_MAX];
    bool online;
} dbuser_t;

typedef struct 
{
    u64 group_id;
    u64 owner_id;
    char displayname[DB_DISPLAYNAME_MAX];
    char desc[DB_DESC_MAX];
    u64* messages;
    size_t n_messages;
} dbgroup_t;

typedef struct 
{
    u64 user_id;
    u64 group_id;
} dbgroup_member_t;

typedef struct 
{
    u64 msg_id;
    u64 user_id;
    u64 group_id;
    char content[DB_MESSAGE_MAX];
    char timestamp[DB_TIMESTAMP_MAX];
} dbmsg_t;

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

#endif // _SERVER_DB_