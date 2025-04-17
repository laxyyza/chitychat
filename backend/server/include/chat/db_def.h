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

#define DB_CONNINTO_LEN     1024
#define DB_ADDRESS_LEN      256

#define DB_INTSTR_MAX       30

typedef struct 
{
    const char*   schema;

    const char*   insert_user;
    const char*   select_user;
    const char*   select_connected_users;
    const char*   select_user_json;
    const char*   delete_user;

    const char*   insert_group;
    const char*   select_user_groups;
    const char*   select_pub_group;
    const char*   delete_group;

    const char*   select_groupmember;
    const char*   insert_pub_groupmember;
    const char*   delete_groupmember;

    const char*   insert_msg;
    const char*   select_msg;
    const char*   select_group_msgs_json;
    const char*   delete_msg;

    const char*   update_user;

    const char*   insert_userfiles;

    const char*   insert_groupmember_code;

    const char*   create_group_code;
    const char*   get_group_code;
    const char*   delete_group_code;

    const char*   insert_session;
    const char*   select_session;

    const char*   insert_remember_token;
    const char*   select_remember_token;
    const char*   update_remember_token;
} server_db_commands_t;

typedef struct eworker eworker_t;
typedef struct client client_t;
typedef struct dbcmd_ctx dbcmd_ctx_t;
typedef struct server_db server_db_t;

typedef const char* (*dbexec_t)(eworker_t* ew, dbcmd_ctx_t* cmd);
typedef void (*dbexec_res_t)(eworker_t* ew, PGresult* res, ExecStatusType res_status, dbcmd_ctx_t* ctx);

#endif // _SERVER_CHAT_DB_DEF_H_
