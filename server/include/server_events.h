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

typedef enum se_status (*se_read_callback_t)(eworker_t* ew, server_event_t* ev);
typedef enum se_status (*se_close_callback_t)(eworker_t* ew, server_event_t* ev);

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
void            server_del_event(eworker_t* ew, server_event_t* se);
void            server_process_event(eworker_t* ew, server_event_t* se);
void            server_wait_for_events(eworker_t* ew);

i32 server_event_add(const server_t* server, server_event_t* ev);
i32 server_event_remove(const server_t* server, const server_event_t* ev);
i32 server_event_rearm(const server_t* server, const server_event_t* ev);

// Handlers 
enum se_status se_accept_conn(eworker_t* ew, server_event_t* ev);
enum se_status se_read_client(eworker_t* ew, server_event_t* ev);
enum se_status se_close_client(eworker_t* ew, server_event_t* ev);

#endif // _SERVER_EVENTS_H_
