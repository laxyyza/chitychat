#ifndef _SERVER_THREAD_MANAGER_H_
#define _SERVER_THREAD_MANAGER_H_

/*
 * server_tm - Server Thread Manager
 */

#include "common.h"
#include <pthread.h>

typedef struct fd_node
{
    i32 fd;
    struct fd_node* next;
} fd_node_t;

typedef struct 
{
    fd_node_t* head;
    fd_node_t* tail;
} server_fdqueue_t;

typedef struct
{
    pthread_t*          threads;
    pthread_mutex_t     queue_mutex;
    pthread_mutex_t     mutex;
    pthread_cond_t      cond;
    server_fdqueue_t    queue;
    size_t              size;
} server_tm_t;

bool server_tm_init_thread_pool(server_t* server, size_t size);
void server_tm_enqueue_fd(server_t* server, const i32 fd);
void server_tm_del_thread_pool(server_t* server);

#endif // _SERVER_THREAD_MANAGER_H_