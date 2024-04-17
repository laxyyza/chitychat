#include "server_timer.h"
#include "server.h"

static enum se_status
timer_user_session(server_t* server, server_timer_t* timer)
{
    session_t* session = timer->data.session;

    debug("Client session for user_id:%u expired %zu times, id: %u.\n", 
          session->user_id, timer->exp, session->session_id);
    server_del_user_session(server, session);

    return SE_CLOSE;
}

static enum se_status
timer_ut(server_thread_t* th, server_timer_t* timer)
{
    upload_token_t* ut = timer->data.ut;

    debug("Upload token for user_id:%u expired %zu times: %u\n", 
          ut->user_id, timer->exp, ut->token);
    /* Set ut->timerfd to 0 so `server_del_upload_token()` won't
     * free this event 
     */
    ut->timerfd = 0;
    server_del_upload_token(th, ut);

    return SE_CLOSE;
}

static enum se_status
server_timer_exp(server_thread_t* th, server_timer_t* timer)
{
    enum se_status ret;

    switch (timer->type)
    {
        case TIMER_CLIENT_SESSION:
            ret = timer_user_session(th->server, timer);
            break;
        case TIMER_UPLOAD_TOKEN:
            ret = timer_ut(th, timer);
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
se_timer_read(server_thread_t* th, server_event_t* ev)
{
    enum se_status ret;
    server_timer_t* timer = ev->data;
    u64 exp;

    if (read(timer->fd, &exp, sizeof(u64)) == -1)
    {
        error("read timer->fd (%d): %s\n", timer->fd, ERRSTR);
        return SE_ERROR;
    }
    timer->exp += exp;

    ret = server_timer_exp(th, timer);
    if (ret == SE_CLOSE)
        ev->keep_data = true;
    return ret;
}

enum se_status
se_timer_close(server_thread_t* th, server_event_t* ev)
{
    server_close_timer(th, ev->data, ev->keep_data);
    return SE_OK;
}

server_timer_t*     
server_addtimer(server_thread_t* th, i32 seconds, i32 flags, 
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
    timer->data = *data;

    switch (timer->type)
    {
        case TIMER_CLIENT_SESSION:
            timer->data.session->timerfd = timer->fd;
            break;
        case TIMER_UPLOAD_TOKEN:
            timer->data.ut->timerfd = timer->fd;
            break;
    }

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

    if (server_new_event(th->server, timer->fd, timer, se_timer_read, se_timer_close) == NULL)
        goto error;

    debug("New timer for %ds, flags:0x%x, type:%d\n",
          timer->seconds, timer->flags, timer->type);

    return timer;
error:;
    server_close_timer(th, timer, 0);
    return NULL;
}

i32                 
server_timer_set(server_timer_t* timer, i32 seconds)
{
    i32 ret;
    struct itimerspec its;

    its.it_value.tv_sec = seconds;
    its.it_value.tv_nsec = 0;
    its.it_interval.tv_nsec = 0;
    if (timer->flags & TIMER_ONCE)
        its.it_interval.tv_sec = 0;
    else
        its.it_interval.tv_sec = seconds;

    ret = timerfd_settime(timer->fd, 0, &its, NULL);
    if (ret == -1)
        error("timerfd_settime(%d, %ds): %s\n",
              timer->fd, seconds, ERRSTR);
    else
        timer->seconds = seconds;

    return ret;
}

i32                 
server_timer_get(server_timer_t* timer)
{
    i32 ret;
    struct itimerspec its;

    ret = timerfd_gettime(timer->fd, &its);
    if (ret == -1)
        error("timerfd_gettime(%d): %s\n",
              timer->fd, ERRSTR);
    else
        ret = its.it_value.tv_sec;

    return ret;
}

void
server_close_timer(server_thread_t* th, server_timer_t* timer, bool keep_data)
{
    if (!timer)
        return;

    if (keep_data == false)
        server_timer_exp(th, timer);

    if (close(timer->fd) == -1)
        error("close timer->fd (%d): %s\n", timer->fd, ERRSTR);

    free(timer);
}
