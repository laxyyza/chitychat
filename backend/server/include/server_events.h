#ifndef _SERVER_EVENTS_H_
#define _SERVER_EVENTS_H_

#include "common.h"
#include "server_tm.h"

/*
 *  `se` = "Server Event"
 */

#define FD_EXCLUSIVE    0
#define FD_SHARED       1

#define SE_DONT_CLOSE_FD 0x01

enum se_status 
{
    SE_OK,
    SE_CLOSE,
    SE_ERROR,
};

typedef struct server_event server_event_t;

typedef enum se_status (*se_read_callback_t)(eworker_t* ew, server_event_t* ev);
typedef enum se_status (*se_write_callback_t)(eworker_t* ew, server_event_t* ev);
typedef enum se_status (*se_close_callback_t)(eworker_t* ew, server_event_t* ev);

typedef struct server_event
{
    i32 fd;
    i32 err;
    u32 ep_events;
    u32 listen_events;
    u32 new_listen_events;
    void* data;
    i32 flags;
    se_read_callback_t read;
    se_write_callback_t write;
    se_close_callback_t close;
    const char* debug_name;
    i32 type;
    atomic_int interests;
} server_event_t;

typedef struct 
{
    i32 fd;
    void* data;
    se_read_callback_t read_cb;
    se_write_callback_t write_cb;
    se_close_callback_t close_cb;
    const char* name;
    i32 type;
} add_event_args_t;

server_event_t* server_epoll_add_event(eworker_t* ew, add_event_args_t* args);

void            eworker_del_event(eworker_t* ew, server_event_t* se);
void            server_process_event(eworker_t* ew, server_event_t* se);
void            server_wait_for_events(eworker_t* ew);
void            server_do_free_event(eworker_t* ew, server_event_t* se);

i32 eworker_epoll_rearm(const eworker_t* ew, server_event_t* ev);
i32 server_epoll_rearm_all(const server_t* server, server_event_t* se);

// Handlers 
enum se_status se_accept_conn(eworker_t* ew, server_event_t* ev);
enum se_status se_read_client(eworker_t* ew, server_event_t* ev);
enum se_status se_ssl_accept(UNUSED eworker_t* th, server_event_t* ev);
enum se_status se_close_client(eworker_t* ew, server_event_t* ev);

void    server_make_event_shared(eworker_t* ew, server_event_t* se);

#endif // _SERVER_EVENTS_H_
