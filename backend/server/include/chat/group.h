#ifndef _SERVER_USER_GROUP_H_
#define _SERVER_USER_GROUP_H_

#include "chat/db_def.h"
#include "server_tm.h"
#include "server_client.h"

typedef struct 
{
    u32     group_id;
    u32     owner_id;
    char    displayname[DB_DISPLAYNAME_MAX];
    char    desc[DB_DESC_MAX];
    bool    public;
    char    created_at[DB_TIMESTAMP_MAX];
} dbgroup_t;

typedef struct 
{
    u32     user_id;
    u32     group_id;
    char    join_date[DB_TIMESTAMP_MAX];
    i32     flags;
} dbgroup_member_t;

typedef struct dbmsg
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
    char    invite_code[DB_GROUP_CODE_MAX];
    u32     group_id;
    i32     uses;
    i32     max_uses;
} dbgroup_code_t;

const char* server_group_create(eworker_t* ew, 
                                client_t* client, 
                                json_object* payload, 
                                json_object* respond_json);

const char* server_client_groups(eworker_t* ew, 
                                 client_t* client, 
                                 json_object* payload,
                                 json_object* respond_json);

const char* server_get_all_groups(eworker_t* ew, 
                                  client_t* client, 
                                  json_object* payload,
                                  json_object* respond_json);

const char* server_join_group(eworker_t* ew, 
                              client_t* client, 
                              json_object* payload, 
                              json_object* respond_json);

const char* server_get_send_group_msg(eworker_t* ew,
                                      const dbmsg_t* msg);

const char* server_group_msg(eworker_t* ew, 
                             client_t* client, 
                             json_object* payload, 
                             json_object* respond_json);

const char* server_get_group_msgs(eworker_t* ew, 
                           client_t* client, 
                           json_object* payload, 
                           json_object* respond_json);

const char* server_create_group_code(eworker_t* ew, 
                                     client_t* client,
                                     json_object* payload, 
                                     json_object* respond_json);

const char* server_join_group_code(eworker_t* ew,
                                   client_t* client,
                                   json_object* payload,
                                   json_object* respond_json);

const char* server_get_group_codes(eworker_t* ew,
                                   client_t* client,
                                   json_object* payload,
                                   json_object* respond_json);

const char* server_delete_group_code(eworker_t* ew,
                                     client_t* client, 
                                     json_object* payload,
                                     json_object* resp);

const char* server_delete_group_msg(eworker_t* ew,
                                    client_t* client,
                                    json_object* payload,
                                    json_object* respond_json);

const char* server_delete_group(eworker_t* ew,
                                client_t* client, 
                                json_object* payload,
                                json_object* resp_json);

const char* server_get_group_member_ids(eworker_t* ew,
                                  client_t* client, 
                                  json_object* payload, 
                                  json_object* resp_json);

#endif // _SERVER_USER_GROUP_H_
