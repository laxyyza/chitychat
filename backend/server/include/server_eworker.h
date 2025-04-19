/*
 * eworker - Event Worker
 *
 *  Process sync/async events
 */

#ifndef _SERVER_EVENT_WORKER_H_
#define _SERVER_EVENT_WORKER_H_

#include "chat/db.h"
#include "server_redis.h"
#include <poll.h>

typedef struct client client_t;
typedef struct eworker eworker_t;

#define THREAD_NAME_LEN 32
#define EWORKER_MAX_EVENTS 16
#define PFDS_COUNT 2

typedef void (*ew_callback_t)(eworker_t* ew, client_t* client, PGresult* res, void* data);

typedef struct eworker
{
    pthread_t   pth;
    pid_t       tid;
    server_db_t db;
    server_redis_t redis;
    char        name[THREAD_NAME_LEN];
    server_t*   server;
    struct epoll_event ep_events[EWORKER_MAX_EVENTS];
    struct pollfd pfds[PFDS_COUNT];
    bool        ignore_http_free;
} server_eworker_t, eworker_t;

void* eworker_main(void* arg);
bool server_eworker_init(eworker_t* ew);
void server_eworker_async_run(eworker_t* ew);
void server_eworker_cleanup(eworker_t* ew);

#endif // _SERVER_EVENT_WORKER_H_
