/*
 * eworker - Event Worker
 *
 *  Process sync/async events
 */

#ifndef _SERVER_EVENT_WORKER_H_
#define _SERVER_EVENT_WORKER_H_

#include "chat/db.h"

typedef struct client client_t;
typedef struct eworker eworker_t;

#define THREAD_NAME_LEN 32

typedef void (*ew_callback_t)(eworker_t* ew, client_t* client, PGresult* res, void* data);

typedef struct eworker
{
    pthread_t   pth;
    pid_t       tid;
    server_db_t db;
    char        name[THREAD_NAME_LEN];
    server_t*   server;
} server_eworker_t, eworker_t;

bool server_create_eworker(server_t* server, eworker_t* ew, size_t i);
bool server_eworker_init(eworker_t* ew);
void server_eworker_async_run(eworker_t* ew);
void server_eworker_cleanup(eworker_t* ew);

#endif // _SERVER_EVENT_WORKER_H_
