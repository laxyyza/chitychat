#define _GNU_SOURCE
#include "server_tm.h"
#include "server.h"
#include <signal.h>
#include <stdio.h>
#include <sys/time.h>

// static i32 server_dequeue_fd()

static i32 server_tm_dequeue(server_tm_t* tm)
{
    server_fdqueue_t* queue = &tm->queue;

    pthread_cond_wait(&tm->cond, &tm->queue_mutex);
    if (queue->head == NULL)
    {
        warn("Q IS NULL!\n");
        return -1;
    }

    fd_node_t* temp = queue->head;
    const i32 fd = temp->fd;
    queue->head = queue->head->next;
    if (queue->head == NULL)
        queue->tail = NULL;
    free(temp);
    return fd;
}

void io_sig(int signum)
{
}

static void* server_tm_io_handler(void* arg)
{
    server_t* server = arg;
    server_tm_t* tm = &server->tm;
    pid_t tid = gettid();

    // signal(SIGTERM, io_sig);

    debug("TID %d up and running!\n", tid);

    while (server->running)
    {
        const i32 fd = server_tm_dequeue(tm);
        if (fd == -1)
            continue;

        struct timeval start, end;
        double elapsed;

        gettimeofday(&start, NULL);

        server_read_fd(server, fd);

        gettimeofday(&end, NULL);

        elapsed = (end.tv_sec - start.tv_sec) * 1000.0; // seconds to milliseconds
        elapsed += (end.tv_usec - start.tv_usec) / 1000.0; // microseconds to milliseconds

        info("Processing %d took: %fms\n", fd, elapsed);

        struct epoll_event ev = {
            .data.fd = fd,
            .events = EPOLLIN | EPOLLRDHUP | EPOLLONESHOT
        };
        if (epoll_ctl(server->epfd, EPOLL_CTL_MOD, fd, &ev) == -1)
        {
            error("epoll_ctl MOD %d: %s\n", fd, ERRSTR);
        }
    }

    debug("TID %d DONE!\n", tid);

    return NULL;
}

static fd_node_t* tm_new_fdnode(const i32 fd)
{
    fd_node_t* node = malloc(sizeof(fd_node_t));
    node->fd = fd;
    node->next = NULL;
    return node;
}

void server_tm_enqueue_fd(server_t* server, const i32 fd)
{
    fd_node_t* node = tm_new_fdnode(fd);
    server_fdqueue_t* queue = &server->tm.queue;

    pthread_mutex_lock(&server->tm.queue_mutex);
    if (queue->tail == NULL)
    {
        queue->head = queue->tail = node;
        goto signal_unlock;
    }
    queue->tail->next = node;
    queue->tail = node;

signal_unlock:
    pthread_cond_signal(&server->tm.cond);
    pthread_mutex_unlock(&server->tm.queue_mutex);
}

bool server_tm_init_thread_pool(server_t* server, size_t size)
{
    server_tm_t* tm = &server->tm; 
    tm->size = size;
    tm->threads = calloc(size, sizeof(pthread_t));
    pthread_mutex_init(&tm->mutex, NULL);
    pthread_mutex_init(&tm->queue_mutex, NULL);
    pthread_cond_init(&tm->cond, NULL);

    for (size_t i = 0; i < size; i++)
    {
        pthread_t* th = tm->threads + i;
        i32 ret = pthread_create(th, NULL, server_tm_io_handler, server);
        char name[64];
        snprintf(name, 64, "io_%zu", i);
        pthread_setname_np(*th, name);
        // debug("pthread_create: %d\n", ret);
    }

    return true;
}

void server_tm_del_thread_pool(server_t* server)
{
    pthread_cond_broadcast(&server->tm.cond);

    for (size_t i = 0; i < server->tm.size; i++)
    {
        pthread_t th = server->tm.threads[i];
        pthread_cond_broadcast(&server->tm.cond);
        // pthread_kill(server->tm.threads[i], )
        pthread_cond_signal(&server->tm.cond);
    }

    pthread_mutex_destroy(&server->tm.mutex);
    pthread_mutex_destroy(&server->tm.queue_mutex);
    pthread_cond_destroy(&server->tm.cond);
}