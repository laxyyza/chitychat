#ifndef _SERVER_EVCB_H_
#define _SERVER_EVCB_H_

/*
 * evcb - "EVent Circular Buffer"
 * Circular/Ring Buffer Data structure
 */

#include "common.h"

typedef struct 
{
    i32 fd;
    u32 ev;
} ev_t;

typedef struct 
{
    ev_t*   buf_begin;
    ev_t*   buf_end;
    size_t  size;
    size_t  count;
    ev_t*   read;
    ev_t*   write;
} evcb_t;

bool server_init_evcb(server_t* server, size_t size);
i32  server_evcb_enqueue(evcb_t* cb, i32 fd, u32 ev);
i32  server_evcb_dequeue(evcb_t* cb, ev_t* ev);
void server_evcb_clear(evcb_t* cb);
void server_evcb_free(evcb_t* cb);

#endif // _SERVER_EVCB_H_
