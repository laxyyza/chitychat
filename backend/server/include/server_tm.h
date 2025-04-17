#ifndef _SERVER_THREAD_MANAGER_H_
#define _SERVER_THREAD_MANAGER_H_

/*
 * TM - "Thread Manager"
 */

#include "common.h"
#include <pthread.h>
#include "chat/db_def.h"

typedef struct 
{
    eworker_t*      workers;    /* Event Workers; Threads */
    size_t          n_workers;

    pthread_mutex_t mutex;
    pthread_cond_t  cond;
} server_tm_t;

void*   tm_worker(void* arg);
bool    server_init_tm(server_t* server, i32 n_threads);
void    server_tm_shutdown(server_t* server);
i32     server_tm_system_threads(void);

void tm_lock(server_tm_t* tm);
void tm_unlock(server_tm_t* tm);
#endif // _SERVER_THREAD_MANAGER_H_
