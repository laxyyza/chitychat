#ifndef _SERVER_TIMER_H_
#define _SERVER_TIMER_H_

#include "common.h"
#include "server_upload_token.h"

#define MINUTES(x)  (x * 60)
#define HOURS(x)    (x * 3600)

#define TIMER_ONCE  0x80

enum timer_type
{
    TIMER_CLIENT_SESSION,
    TIMER_UPLOAD_TOKEN
};

union timer_data
{
    session_t* session;
    upload_token_t* ut;
};

typedef struct 
{
    i32 fd;
    i32 seconds;
    i32 flags;
    u64 exp;
    enum timer_type type;
    union timer_data data;
} server_timer_t;

server_timer_t*     server_addtimer(server_t* server, i32 seconds, i32 flags, 
                                    enum timer_type type, union timer_data* data, 
                                    size_t size);
i32                 server_timer_set(server_timer_t* timer, i32 seconds);
i32                 server_timer_get(server_timer_t* timer);
void                server_close_timer(server_timer_t* timer);

#endif // _SERVER_TIMER_H_
