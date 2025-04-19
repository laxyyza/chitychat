#include "server_redis.h"
#include "server_events.h"
#include "server.h"
#include <poll.h>

// TODO: Make TTL customizable.
#define TTL 3600

static void 
redis_connect_cb(const redisAsyncContext* c, i32 status)
{
    eworker_t* ew = c->data;

    if (status != REDIS_OK)
    {
        fatal("Redis Connection Failed: %s\n", c->errstr);
        ew->server->running = false;
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
redis_callback(redisAsyncContext* c, UNUSED redisReply* r, UNUSED void* privData)
{
    eworker_t* ew = c->data;
    ew->redis.cmds--;
}

static void 
redis_add_read(server_redis_t* r)
{
    r->pfd->events |= POLLIN;
}

static void 
redis_del_read(server_redis_t* r)
{
    r->pfd->events &= ~POLLIN;
}

static void 
redis_add_write(server_redis_t* r)
{
    r->pfd->events |= POLLOUT;
}

static void 
redis_del_write(server_redis_t* r)
{
    r->pfd->events &= ~POLLOUT;
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

bool 
server_init_redis(eworker_t* ew)
{
    server_t* server = ew->server;
    server_redis_t* r = &ew->redis;
    r->ew = ew;
    r->c = redisAsyncConnect(server->conf.redis_ip, server->conf.redis_port);
    if (r->c->err)
    {
        fatal("redisAsyncConnect: %s\n", r->c->errstr);
        redisAsyncFree(r->c);
        return false;
    }
    redisAsyncContext* c = r->c;
    c->data = ew;

    r->pfd = &ew->pfds[1];

    c->ev.data = r;
    c->ev.addRead = (void*)redis_add_read;
    c->ev.delRead = (void*)redis_del_read;
    c->ev.addWrite = (void*)redis_add_write;
    c->ev.delWrite = (void*)redis_del_write;
    c->ev.cleanup = (void*)redis_cleanup;
    c->ev.scheduleTimer = (void*)redis_sched_timer;

    redisAsyncSetConnectCallback(r->c, redis_connect_cb);
    redisAsyncSetDisconnectCallback(r->c, redis_disconnect_cb);

    return true;
}

void 
server_deinit_redis(UNUSED server_t* server)
{
    // TODO: Implement
}

void 
server_redis_set_session(server_redis_t* r, const char* session_uuid, u32 user_id)
{
    redisAsyncCommand(r->c, 
                      (redisCallbackFn*)redis_callback, 
                      NULL, 
                      "SET session:%s %u EX %d", 
                      session_uuid, user_id, TTL);
    r->cmds++;
}

static void 
get_session_cb(redisAsyncContext* c, redisReply* r, redis_cb_data_t* data)
{
    eworker_t* ew = c->data;
    ew->redis.cmds -= 2; // GET and EXPIRE commands.
    
    if (r->type != REDIS_REPLY_STRING)
        data->data.found = false;
    else
    {
        char* endptr;
        data->data.user_id = strtoul(r->str, &endptr, 10);
        data->data.found = true;
    }

    data->callback(ew, &data->data);
}

void 
server_redis_get_session(server_redis_t* r, const char* session_uuid, redis_cb_data_t* data)
{
    redisAsyncCommand(r->c, 
                      (void*)get_session_cb, 
                      data, 
                      "GET session:%s", session_uuid);

    redisAsyncCommand(r->c, 
                      NULL, 
                      NULL, 
                      "EXPIRE session:%s %d", session_uuid, TTL);

    r->cmds += 2;
}

redis_cb_data_t* 
server_redis_get_cb_data(server_redis_t* r)
{
    u32 data_idx = r->ring_idx++;
    redis_cb_data_t* ret = r->ring_data + (data_idx % SERVER_REDIS_RING_SIZE);

    return ret;
}
