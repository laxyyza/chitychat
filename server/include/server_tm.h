#ifndef _SERVER_THREAD_MANAGER_H_
#define _SERVER_THREAD_MANAGER_H_

#include "common.h"
#include <pthread.h>
#include "chat/db_def.h"

#define THREAD_NAME_LEN 32

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
    pthread_t   pth;
    pid_t       tid;
    server_db_t db;
    server_t*   server;
    char name[THREAD_NAME_LEN];
} server_thread_t;

typedef struct 
{
    server_queue_t  q;
    server_thread_t* threads;
    size_t          n_threads;
    u8              shutdown;

    pthread_mutex_t mutex;
    pthread_cond_t  cond;
} server_tm_t;

bool    server_init_tm(server_t* server, i32 n_threads);
bool    server_tm_init_thread(server_t* server, server_thread_t* th, size_t i);
void    server_tm_shutdown(server_t* server);
i32     server_tm_system_threads(void);

void            server_tm_enq(server_tm_t* tm, i32 fd, u32 ev);
server_job_t*   server_tm_deq(server_tm_t* tm);

void tm_lock(server_tm_t* tm);
void tm_unlock(server_tm_t* tm);
#endif // _SERVER_THREAD_MANAGER_H_
