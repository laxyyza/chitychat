#ifndef _SERVER_EVENTS_H_
#define _SERVER_EVENTS_H_

#include "common.h"

/*
 *  `se` = "Server Event"
 */

enum se_status 
{
    SE_OK,
    SE_CLOSE,
    SE_ERROR,
};

typedef struct server_event server_event_t;

typedef enum se_status (*se_close_callback_t)(server_t* server, server_event_t* ev);
typedef enum se_status (*se_read_callback_t)(server_t* server, server_event_t* ev);
// TODO: Use server_event_t
typedef struct server_event
{
    i32 fd;
    u32 ep_events;
    void* data;
    se_read_callback_t read;
    se_close_callback_t close;

    struct server_event* next;
    struct server_event* prev;
} server_event_t;

server_event_t* server_new_event(server_t* server, i32 fd, void* data, 
                                se_read_callback_t read_callback, 
                                se_close_callback_t close_callback);
server_event_t* server_get_event(server_t* server, i32 fd);
void            server_del_event(server_t* server, server_event_t* se);

int server_ep_addfd(server_t* server, i32 fd);
int server_ep_delfd(server_t* server, i32 fd);

// Handlers 
enum se_status se_accept_conn(server_t* server, server_event_t* ev);
enum se_status se_read_client(server_t* server, server_event_t* ev);
enum se_status se_close_client(server_t* server, server_event_t* ev);

#endif // _SERVER_EVENTS_H_
