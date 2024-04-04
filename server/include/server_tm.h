#ifndef _SERVER_THREAD_MANAGER_H_
#define _SERVER_THREAD_MANAGER_H_

#include "common.h"
#include <pthread.h>

typedef struct server_job
{
    i32 fd;
    u32 ev;

    struct server_job* next;
} server_job_t;

typedef struct 
{
    server_job_t* front; 
    server_job_t* rear;
} server_queue_t;

typedef struct 
{
    server_queue_t  q;
    pthread_t*      threads;
    size_t          n_threads;
    u8              shutdown;

    pthread_mutex_t mutex;
    pthread_cond_t  cond;
} server_tm_t;

bool    server_tm_init(server_t* server, size_t n_threads);
void    server_tm_shutdown(server_t* server);

void            server_tm_enq(server_tm_t* tm, i32 fd, u32 ev);
server_job_t*   server_tm_deq(server_tm_t* tm);

void tm_lock(server_tm_t* tm);
void tm_unlock(server_tm_t* tm);
#endif // _SERVER_THREAD_MANAGER_H_
