#ifndef _SERVER_THREAD_MANAGER_H_
#define _SERVER_THREAD_MANAGER_H_

/*
 * TM - "Thread Manager"
 */

#include "common.h"
#include <pthread.h>
#include "chat/db_def.h"

#define TM_STATE_INIT       0x01 /* Thread Manager initialized */
#define TM_STATE_SHUTDOWN   0x80 /* Thread Manager shutting down */

typedef struct 
{
    eworker_t*      workers;    /* Event Workers; Threads */
    size_t          n_workers;
    u8              state;

    pthread_mutex_t mutex;
    pthread_cond_t  cond;
} server_tm_t;

bool    server_init_tm(server_t* server, i32 n_threads);
void    server_tm_shutdown(server_t* server);
i32     server_tm_system_threads(void);

void tm_lock(server_tm_t* tm);
void tm_unlock(server_tm_t* tm);
#endif // _SERVER_THREAD_MANAGER_H_
