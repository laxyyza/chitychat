#ifndef _SERVER_EVENTS_H_
#define _SERVER_EVENTS_H_

#include "common.h"
#include "server_tm.h"

/*
 *  `se` = "Server Event"
 */

#define DEFAULT_EPEV (EPOLLIN | EPOLLRDHUP | EPOLLONESHOT)

enum se_status 
{
    SE_OK,
    SE_CLOSE,
    SE_ERROR,
};

typedef struct server_event server_event_t;

typedef enum se_status (*se_read_callback_t)(server_thread_t* th, server_event_t* ev);
typedef enum se_status (*se_close_callback_t)(server_thread_t* th, server_event_t* ev);
// TODO: Use server_event_t
typedef struct server_event
{
    i32 fd;
    i32 err;
    u32 ep_events;
    u32 listen_events;
    void* data;
    bool  keep_data;
    se_read_callback_t read;
    se_close_callback_t close;
} server_event_t;

server_event_t* server_new_event(server_t* server, i32 fd, void* data, 
                                se_read_callback_t read_callback, 
                                se_close_callback_t close_callback);
server_event_t* server_get_event(server_t* server, i32 fd);
void            server_del_event(server_thread_t* th, server_event_t* se);

i32 server_ep_addfd(server_t* server, i32 fd);
i32 server_ep_delfd(server_t* server, i32 fd);
i32 server_ep_rearm(server_t* server, i32 fd);

// Handlers 
enum se_status se_accept_conn(server_thread_t* th, server_event_t* ev);
enum se_status se_read_client(server_thread_t* th, server_event_t* ev);
enum se_status se_close_client(server_thread_t* th, server_event_t* ev);

#endif // _SERVER_EVENTS_H_
