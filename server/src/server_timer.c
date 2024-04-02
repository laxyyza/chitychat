#include "server_timer.h"
#include "server.h"
#include <sys/timerfd.h>

static enum se_status
timer_client_session(server_t* server, server_timer_t* timer)
{
    session_t* session = timer->data.session;

    debug("Client session for user_id:%u expired, id: %u.\n", 
          session->user_id, session->session_id);
    server_del_client_session(server, session);

    return SE_CLOSE;
}

static enum se_status
timer_ut(server_t* server, server_timer_t* timer)
{
    upload_token_t* ut = timer->data.ut;

    debug("Upload token for user_id:%u expired: %u\n", 
          ut->user_id, ut->token);
    server_del_upload_token(server, ut);

    return SE_CLOSE;
}

enum se_status
se_timer_read(server_t* server, server_event_t* ev)
{
    enum se_status ret = SE_OK;
    server_timer_t* timer = ev->data;
    u64 exp;

    if (read(timer->fd, &exp, sizeof(u64)) == -1)
    {
        error("read timer->fd (%d): %s\n", timer->fd, ERRSTR);
        return SE_ERROR;
    }
    timer->exp += exp;

    switch (timer->type)
    {
        case TIMER_CLIENT_SESSION:
            ret = timer_client_session(server, timer);
            break;
        case TIMER_UPLOAD_TOKEN:
            ret = timer_ut(server, timer);
            break;
        default:
        {
            warn("Not handled timer type: %d\n", timer->type);
            ret = SE_ERROR;
            break;
        }
    }

    return ret;
}

enum se_status
se_timer_close(UNUSED server_t* server, server_event_t* ev)
{
    server_close_timer(ev->data);
    return SE_OK;
}

server_timer_t*     
server_addtimer(server_t* server, i32 seconds, i32 flags, 
                enum timer_type type, union timer_data* data,
                size_t size)
{
    struct itimerspec it;
    server_timer_t* timer = NULL; 

    timer = calloc(1, sizeof(server_timer_t));

    timer->fd = timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC);
    if (timer->fd == -1)
    {
        error("timerfd_create: %s\n", ERRSTR);
        goto error;
    }
    timer->seconds = seconds;
    timer->flags = flags;
    timer->type = type;
    if (size > sizeof(union timer_data))
        size = sizeof(union timer_data);
    memcpy(&timer->data, data, size);

    it.it_value.tv_sec = seconds;
    it.it_value.tv_nsec = 0;
    it.it_interval.tv_nsec = 0;
    if (flags & TIMER_ONCE)
        it.it_interval.tv_sec = 0;
    else
        it.it_interval.tv_sec = seconds;


    if (timerfd_settime(timer->fd, 0, &it, NULL) == -1)
    {
        error("timerfd_settime: %s\n", ERRSTR);
        goto error;
    }

    if (server_new_event(server, timer->fd, timer, se_timer_read, se_timer_close) == NULL)
        goto error;

    debug("New timer for %ds, flags:0x%x, type:%d\n",
          timer->seconds, timer->flags, timer->type);

    return timer;
error:;
    if (timer->fd != -1)
        close(timer->fd);
    free(timer);
    return NULL;
}

i32                 
server_timer_set(UNUSED server_timer_t* timer, UNUSED i32 seconds)
{
    return -1;
}

i32                 
server_timer_get(UNUSED server_timer_t* timer)
{
    return -1;
}

void
server_close_timer(server_timer_t* timer)
{
    if (!timer)
        return;

    if (close(timer->fd) == -1)
        error("close timer->fd (%d): %s\n", timer->fd, ERRSTR);

    free(timer);
}
