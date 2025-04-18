#include "server_redis.h"
#include "server_events.h"
#include "server.h"

// TODO: Make TTL customizable.
#define TTL 3600

static void 
redis_connect_cb(const redisAsyncContext* c, i32 status)
{
    server_t* server = c->data;

    if (status != REDIS_OK)
    {
        fatal("Redis Connection Failed: %s\n", c->errstr);
        server->running = false;
    }
}

static void 
redis_disconnect_cb(const redisAsyncContext* c, i32 status)
{
    if (status != REDIS_OK)
    {
        fatal("Redis Disconnected: %d\n", c->err);
    }
}

static void 
redis_callback(UNUSED redisAsyncContext* c, redisReply* r, UNUSED void* privData)
{
    if (r == NULL)
        return;

    info("elements: %lu\n", r->elements);
}

static void 
redis_add_read(server_redis_t* r)
{
    r->ev->new_listen_events |= EPOLLIN;

    if (r->ev->new_listen_events != r->ev->listen_events)
        server_epoll_rearm(r->server, r->ev);
}

static void 
redis_del_read(server_redis_t* r)
{
    r->ev->new_listen_events &= ~EPOLLIN;
}

static void 
redis_add_write(server_redis_t* r)
{
    r->ev->new_listen_events |= EPOLLOUT;

    if (r->ev->new_listen_events != r->ev->listen_events)
        server_epoll_rearm(r->server, r->ev);
}

static void 
redis_del_write(server_redis_t* r)
{
    r->ev->new_listen_events &= ~EPOLLOUT;
}

static void 
redis_cleanup(UNUSED server_redis_t* r)
{
    warn("TODO: Implement: redis_cleanup()!\n");
}

static void 
redis_sched_timer(UNUSED server_redis_t* r, struct timeval* tv)
{
    warn("TODO: implement: redis_sched_timer: %ds\n", tv->tv_sec);
}

static enum se_status
redis_read(UNUSED eworker_t* ew, server_event_t* ev)
{
    server_redis_t* r = ev->data;

    redisAsyncHandleRead(r->c);

    return SE_OK;
}

static enum se_status
redis_write(UNUSED eworker_t* ew, server_event_t* ev)
{
    server_redis_t* r = ev->data;

    redisAsyncHandleWrite(r->c);

    return SE_OK;
}

static enum se_status
redis_close(UNUSED eworker_t* ew, UNUSED server_event_t* ev)
{
    warn("TODO: Implement redis_close()!\n");
    return SE_OK;
}

bool 
server_init_redis(server_t* server)
{
    server_redis_t* r = &server->redis;
    r->server = server;
    r->c = redisAsyncConnect(server->conf.redis_ip, server->conf.redis_port);
    if (r->c->err)
    {
        fatal("redisAsyncConnect: %s\n", r->c->errstr);
        redisAsyncFree(r->c);
        return false;
    }
    redisAsyncContext* c = r->c;
    c->data = server;

    r->ev = server_epoll_add_event(server, c->c.fd, r, 
                                   redis_read, 
                                   redis_write, 
                                   redis_close);

    c->ev.data = r;
    c->ev.addRead = (void*)redis_add_read;
    c->ev.delRead = (void*)redis_del_read;
    c->ev.addWrite = (void*)redis_add_write;
    c->ev.delWrite = (void*)redis_del_write;
    c->ev.cleanup = (void*)redis_cleanup;
    c->ev.scheduleTimer = (void*)redis_sched_timer;

    redisAsyncSetConnectCallback(r->c, redis_connect_cb);
    redisAsyncSetDisconnectCallback(r->c, redis_disconnect_cb);

    atomic_init(&r->ring_idx, 0);

    return true;
}

void 
server_deinit_redis(UNUSED server_t* server)
{
    // TODO: Implement
}

void 
server_redis_set_session(server_t* server, const char* session_uuid, u32 user_id)
{
    redisAsyncCommand(server->redis.c, 
                      (redisCallbackFn*)redis_callback, 
                      NULL, 
                      "SET session:%s %u EX %d", 
                      session_uuid, user_id, TTL);
}

static void 
get_session_cb(redisAsyncContext* c, redisReply* r, redis_cb_data_t* data)
{
    if (r->type != REDIS_REPLY_STRING)
        data->data.found = false;
    else
    {
        char* endptr;
        data->data.user_id = strtoul(r->str, &endptr, 10);
        data->data.found = true;
    }

    data->callback(c->data, &data->data);
}

void 
server_redis_get_session(server_t* server, const char* session_uuid, redis_cb_data_t* data)
{
    redisAsyncCommand(server->redis.c, 
                      (void*)get_session_cb, 
                      data, 
                      "GET session:%s", session_uuid);

    redisAsyncCommand(server->redis.c, 
                      NULL, 
                      NULL, 
                      "EXPIRE session:%s %d", session_uuid, TTL);
}

redis_cb_data_t* 
server_redis_get_cb_data(server_redis_t* r)
{
    u32 data_idx = atomic_fetch_add(&r->ring_idx, 1);
    redis_cb_data_t* ret = r->ring_data + (data_idx % SERVER_REDIS_RING_SIZE);

    return ret;
}
