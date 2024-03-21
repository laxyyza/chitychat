#ifndef _SERVER_UPLOAD_TOKEN_H_
#define _SERVER_UPLOAD_TOKEN_H_

#include "common.h"
#include "server_db.h"
#include "server_client.h"

enum upload_token_type 
{
    UT_USER_PFP,
    UT_MSG_ATTACHMENT
};

typedef struct 
{
    dbmsg_t msg;
    u32 token;
    u32 total;
    u32 current;
} tmp_msg_t;

typedef struct upload_token
{
    enum upload_token_type type;
    u32 token;
    union {
        u32 user_id;
        tmp_msg_t msg_state;
    };
    i32 timerfd;

    struct upload_token* next;
    struct upload_token* prev;
} upload_token_t;

upload_token_t* server_new_upload_token(server_t* server, u32 user_id);
upload_token_t* server_new_upload_token_attach(server_t* server, dbmsg_t* msg);
upload_token_t* server_get_upload_token(server_t* server, u32 token);
ssize_t         server_send_upload_token(client_t* client, const char* packet_type, upload_token_t* ut);
void            server_del_upload_token(server_t* server, upload_token_t* upload_token);

#endif // _SERVER_UPLOAD_TOKEN_H_