#ifndef _SERVER_DB_
#define _SERVER_DB_

#include "common.h"

#include <sqlite3.h>

#define DB_USERNAME_MAX 50
#define DB_DISPLAYNAME_MAX 50
#define DB_MESSAGE_MAX  512
#define DB_BIO_MAX 256

typedef struct 
{
    u64 user_id;
    char username[DB_USERNAME_MAX];
    char displayname[DB_DISPLAYNAME_MAX];
    char bio[DB_BIO_MAX];
    bool online;
} dbuser_t;

typedef struct 
{
    u64 group_id;
    u64 owner_id;
    char displayname[DB_DISPLAYNAME_MAX];
    u64* messages;
    size_t n_messages;
} dbgroup_t;

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
} server_db_t;

bool        server_db_init(server_t* server);

dbuser_t*   server_db_get_user_from_id(server_t* server, u64 user_id);
dbuser_t*   server_db_get_user_from_name(server_t* server, const char* username);
dbuser_t*   server_db_get_users_from_group(server_t* server, u64 group_id, u32* n);
bool        server_db_insert_user(server_t* server, dbuser_t* user);

dbgroup_t*  server_db_get_group(server_t* server, u64 group_id);
dbgroup_t*  server_db_get_user_groups(server_t* server, u64 user_id, u32* n);
bool        server_db_insert_group(server_t* server, dbgroup_t* group);

dbmsg_t*    server_db_get_msg(server_t* server, u64 msg_id);
dbmsg_t*    server_db_get_msgs_from_group(server_t* server, u64 group_id, u32 max, u32* n);
bool        server_db_insert_msg(server_t* server, dbmsg_t* msg);

#endif // _SERVER_DB_