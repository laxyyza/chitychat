#ifndef _SERVER_REDIS_H_
#define _SERVER_REDIS_H_

#include "common.h"
#include "server_events.h"
#include "server_nats.h"

#include <hiredis/hiredis.h>
#include <hiredis/async.h>
#include <stdatomic.h>

// TODO: Make it thread-safe, since redisAsyncContext is not thread-safe.

#define SERVER_REDIS_RING_SIZE 512

typedef struct 
{
    http_t* http;
    client_t* client;
    u32 user_id;
    char subject[SUBJECT_LEN];
    bool found;
} get_session_data_t;

typedef void (*redis_get_session_cb_t)(server_t* server, get_session_data_t* data);

typedef struct 
{
    get_session_data_t data;
    redis_get_session_cb_t callback;
} redis_cb_data_t;

typedef struct 
{
    redisAsyncContext* c;
    server_event_t* ev;
    server_t* server;

    redis_cb_data_t ring_data[SERVER_REDIS_RING_SIZE];
    atomic_uint     ring_idx;
} server_redis_t;

bool server_init_redis(server_t* server);
void server_deinit_redis(server_t* server);

void server_redis_set_session(server_t* server, const char* session_uuid, u32 user_id);
void server_redis_get_session(server_t* server, const char* session_uuid, redis_cb_data_t* data);
redis_cb_data_t* server_redis_get_cb_data(server_redis_t* r);

#endif // _SERVER_REDIS_H_
