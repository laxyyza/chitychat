#ifndef _SERVER_DB_
#define _SERVER_DB_

#include "common.h"
#include "chat/group.h"
#include "chat/user_login.h"
#include "backend_route.h"
#include "server_auth_token.h"

#define DB_PIPELINE     0x01
#define DB_NONBLOCK     0x02
#define DB_TRY_RECONNECT 0x04
#define DB_DEFAULT      0x00

#define DB_ASYNC_BUSY   0
#define DB_ASYNC_OK     1
#define DB_ASYNC_ERROR -1

#define DB_CTX_NO_JSON   0x01
#define DB_CTX_DONT_FREE 0x02

typedef struct server_config server_config_t;

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

typedef struct 
{
    u32 msg_id;
    u32 group_id;
    const char* attachments_json;
} delete_msg_param_t;

typedef struct 
{
    u32 group_id;
    const char* array_json;
} get_group_codes_param_t;

typedef struct 
{
    u32 group_id;
    u32 owner_id;
    u32 user_id;
} group_owner_param_t;

typedef struct 
{
	u64 group_id;
	u32 limit;
	u32 offset;
} get_group_msgs_param_t;

typedef struct
{
    u32 user_id;
    rtusm_new_t new;
    rtusm_t status;
    const char* pfp_hash;
} rtusm_param_t;

typedef struct 
{
    client_t* client;
    dbuser_t* user;
} session_login_param_t;

typedef struct 
{
    const char* subject;
    http_t* http;
} session_route_t;

typedef struct 
{
    client_t* client;
    auth_callback_t callback;
    u32 user_id;
} remember_token_param_t;

union cmd_param 
{
    user_login_param_t user_login;
    member_ids_param_t member_ids;
    group_msgs_param_t group_msgs;
    get_group_codes_param_t group_codes;
    delete_msg_param_t del_msg;
    group_owner_param_t group_owner;
    rtusm_param_t rtusm;
    dbsession_t*  session;
    json_object* json;
    const char* str;
    void*       ptr;
    u32         group_id;
    u32         user_id;
	get_group_msgs_param_t get_group_msgs;
    session_login_param_t session_login;
    session_route_t session_route;
    client_t* client;
    char session_id[UUID_LEN];
    remember_token_param_t remember_token;
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
bool        server_db_open(server_db_t* db, server_config_t* config, i32 flags);
void        server_db_free(server_t* server);
void        server_db_close(server_db_t* db);

/* Result to structure */
void db_row_to_user(dbuser_t* user, PGresult* res, i32 row);
void db_row_to_group(dbgroup_t* group, PGresult* res, i32 row);

#endif // _SERVER_DB_
