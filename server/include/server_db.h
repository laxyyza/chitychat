#ifndef _SERVER_DB_
#define _SERVER_DB_

#include "common.h"

#include <sqlite3.h>

#define DB_USERNAME_MAX 50
#define DB_DISPLAYNAME_MAX 50
#define DB_MESSAGE_MAX  512
#define DB_BIO_MAX 256
#define DB_DESC_MAX 256
#define DB_PASSWORD_MAX 50

typedef struct 
{
    u64 user_id;
    char username[DB_USERNAME_MAX];
    char displayname[DB_DISPLAYNAME_MAX];
    char bio[DB_BIO_MAX];
    char password[DB_PASSWORD_MAX];
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
} server_db_t;

bool        server_db_open(server_t* server);
void        server_db_close(server_t* server);

dbuser_t*   server_db_get_user_from_id(server_t* server, u64 user_id);
dbuser_t*   server_db_get_user_from_name(server_t* server, const char* username);
dbuser_t*   server_db_get_users_from_group(server_t* server, u64 group_id, u32* n);
bool        server_db_insert_user(server_t* server, dbuser_t* user);

dbuser_t*   server_db_get_group_members(server_t* server, u64 group_id, u32* n);
bool        server_db_user_in_group(server_t* server, u64 group_id, u64 user_id);
bool        server_db_insert_group_member(server_t* server, u64 group_id, u64 user_id);

dbgroup_t*  server_db_get_group(server_t* server, u64 group_id);
dbgroup_t*  server_db_get_all_groups(server_t* server, u32* n);
dbgroup_t*  server_db_get_user_groups(server_t* server, u64 user_id, u32* n);
bool        server_db_insert_group(server_t* server, dbgroup_t* group);

dbmsg_t*    server_db_get_msg(server_t* server, u64 msg_id);
dbmsg_t*    server_db_get_msgs_from_group(server_t* server, u64 group_id, u32 max, u32* n);
bool        server_db_insert_msg(server_t* server, dbmsg_t* msg);

#endif // _SERVER_DB_