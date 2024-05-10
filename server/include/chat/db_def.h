#ifndef _SERVER_CHAT_DB_DEF_H_
#define _SERVER_CHAT_DB_DEF_H_

/*
 * db_def - DataBase Defines
 */

#include "server_crypt.h"
#include <libpq-fe.h>

#define DB_USERNAME_MAX     50
#define DB_DISPLAYNAME_MAX  50
#define DB_MESSAGE_MAX      4096
#define DB_TIMESTAMP_MAX    64
#define DB_BIO_MAX          256
#define DB_DESC_MAX         256
#define DB_PASSWORD_MAX     50
#define DB_PFP_HASH_MAX     SERVER_HASH256_STR_SIZE
#define DB_PFP_NAME_MAX     NAME_MAX
#define DB_MIME_TYPE_LEN    32 
#define DB_GROUP_CODE_MAX   8

#define DB_CONNINTO_LEN     256

#define DB_INTSTR_MAX       30

typedef struct 
{
    char*   schema;
    size_t  schema_len;

    char*   insert_user;
    size_t  insert_user_len;
    char*   select_user;
    size_t  select_user_len;
    char*   select_connected_users;
    size_t  select_connected_users_len;
    char*   delete_user;
    size_t  delete_user_len;

    char*   insert_group;
    size_t  insert_group_len;
    char*   select_group;
    size_t  select_group_len;
    char*   select_pub_group;
    size_t  select_pub_group_len;
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

typedef struct eworker eworker_t;
typedef struct client client_t;
struct dbasync_cmd;

typedef void (*dbexec_t)(eworker_t* ew, struct dbasync_cmd* cmd);

#define DB_ASYNC_OK     0
#define DB_ASYNC_ERROR -1

struct dbasync_cmd
{
    i32       ret;
    client_t* client;
    PGresult* res;
    void*     data;
    dbexec_t  exec;

    struct dbasync_cmd* next;
};

typedef struct 
{
    struct dbasync_cmd* begin;
    struct dbasync_cmd* end;
    struct dbasync_cmd* read;
    struct dbasync_cmd* write;
    size_t size;
    size_t count;
} pipeline_queue_t, plq_t;

typedef struct 
{
    i32     fd;
    i32     flags;
    PGconn* conn;
    plq_t   queue;
    struct {
        struct dbasync_cmd* head;
        struct dbasync_cmd* tail;
    } current;
    const server_db_commands_t* cmd;
} server_db_t;

#endif // _SERVER_CHAT_DB_DEF_H_
