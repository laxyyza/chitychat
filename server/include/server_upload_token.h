#ifndef _SERVER_UPLOAD_TOKEN_H_
#define _SERVER_UPLOAD_TOKEN_H_

#include "common.h"

typedef struct upload_token
{
    u32 token;
    u32 user_id;
    i32 timerfd;

    struct upload_token* next;
    struct upload_token* prev;
} upload_token_t;

upload_token_t* server_new_upload_token(server_t* server, u32 user_id);
upload_token_t* server_get_upload_token(server_t* server, u32 token);
void            server_del_upload_token(server_t* server, upload_token_t* upload_token);

#endif // _SERVER_UPLOAD_TOKEN_H_