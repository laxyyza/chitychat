#include "server_evcb.h"
#include "server.h"

bool 
server_init_evcb(server_t* server, size_t size)
{
    evcb_t* cb = &server->tm.eq;
    cb->buf_begin = malloc(size * sizeof(ev_t));
    if (cb->buf_begin == NULL)
    {
        fatal("malloc() returned NULL!\n");
        return false;
    }
    for (size_t i = 0; i < size; i++)
        cb->buf_begin[i].fd = -1;

    cb->size = size;
    cb->buf_end = cb->buf_begin + size - 1;
    cb->read = cb->buf_begin;
    cb->write = cb->buf_begin;

    return true;
}

i32
server_evcb_enqueue(evcb_t* cb, i32 fd, u32 ev)
{
    if (cb->write->fd != -1)
        return -1;

    cb->write->fd = fd;
    cb->write->ev = ev;
    cb->count++;

    if ((cb->write++ >= cb->buf_end))
        cb->write = cb->buf_begin;
    return 0;
}

i32  
server_evcb_dequeue(evcb_t* cb, ev_t* ev)
{
    if (cb->read->fd == -1)
        return 0;

    ev->fd = cb->read->fd;
    ev->ev = cb->read->ev;
    cb->count--;

    cb->read->fd = -1;
    if ((cb->read++ >= cb->buf_end))
        cb->read = cb->buf_begin;
    return 1;
}

void 
server_evcb_clear(evcb_t* cb)
{
    ev_t ev;
    while (server_evcb_dequeue(cb, &ev));
}

void 
server_evcb_free(evcb_t* cb)
{
    if (!cb || !cb->buf_begin)
        return;

    free(cb->buf_begin);
}
