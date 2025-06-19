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
#define DB_PFP_NAME_MAX     NAME_MAX
#define DB_MIME_TYPE_LEN    32 
#define DB_GROUP_CODE_MAX   8
#define DB_PFP_URL_MAX      1024

#define DB_CONNINTO_LEN     1024
#define DB_ADDRESS_LEN      256

#define DB_INTSTR_MAX       30

typedef struct 
{
    const char* sql;
    const char* filename;
} sql_query_t;

typedef struct 
{
    sql_query_t   schema;

    sql_query_t   insert_user;
    sql_query_t   select_user;
    sql_query_t   select_connected_users;
    sql_query_t   select_user_json;
    sql_query_t   delete_user;

    //sql_query_t   update_user;
    sql_query_t   insert_remember_token;
    sql_query_t   select_remember_token;
    sql_query_t   update_remember_token;

    sql_query_t   insert_login_attempt;
    sql_query_t   insert_remember_token_usage;
} server_db_commands_t;

typedef struct eworker eworker_t;
typedef struct client client_t;
typedef struct dbcmd_ctx dbcmd_ctx_t;
typedef struct server_db server_db_t;

typedef const char* (*dbexec_t)(eworker_t* ew, dbcmd_ctx_t* cmd);
typedef void (*dbexec_res_t)(eworker_t* ew, PGresult* res, ExecStatusType res_status, dbcmd_ctx_t* ctx);

#endif // _SERVER_CHAT_DB_DEF_H_
