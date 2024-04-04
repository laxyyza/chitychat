#include "server_tm.h"
#include "server.h"
#include <pthread.h>

void* 
tm_worker(void* arg)
{
    server_t* server = arg;
    server_tm_t* tm = &server->tm;
    server_job_t* job;
    pid_t tid;

    tid = gettid();

    info("Worker %d up and running!\n", tid);

    while (1)
    {
        job = server_tm_deq(tm);

        if (tm->shutdown == 1)
            break;

        if (job)
        {
            server_ep_event(server, job);
            free(job);
            job = NULL;
        }
    }

    info("Worker %d shutting down...\n", tid);

    return NULL;
}

bool    
server_tm_init(server_t* server, size_t n_threads)
{
    server_tm_t* tm = &server->tm;
    tm->n_threads = n_threads;
    tm->threads = calloc(n_threads, sizeof(pthread_t));
    tm->shutdown = 0;

    pthread_mutex_init(&tm->mutex, NULL);
    pthread_cond_init(&tm->cond, NULL);

    for (size_t i = 0; i < n_threads; i++)
    {
        char thname[32];
        snprintf(thname, 32, "tm_worker%zu", i);
        pthread_t* th = tm->threads + i;

        pthread_create(th, NULL, tm_worker, server);
        pthread_setname_np(*th, thname);
    }

    return true;
}

void    
server_tm_shutdown(server_t* server)
{
    server_tm_t* tm = &server->tm;
    server_job_t* job;

    tm_lock(tm);
    tm->shutdown = 1;

    while ((job = tm->q.front))
    {
        tm->q.front = job->next;
        free(job);
    }
    
    pthread_cond_broadcast(&tm->cond);
    tm_unlock(tm);

    for (size_t i = 0; i < tm->n_threads; i++)
        pthread_join(tm->threads[i], NULL);

    pthread_cond_destroy(&tm->cond);
    pthread_mutex_destroy(&tm->mutex);
}

void            
server_tm_enq(server_tm_t* tm, i32 fd, u32 ev)
{
    server_job_t* job = malloc(sizeof(server_job_t));

    job->fd = fd;
    job->ev = ev;
    job->next = NULL;

    debug("enqueue %d\n", fd);

    tm_lock(tm);
    if (tm->q.front)
    {
        tm->q.rear->next = job;
        tm->q.rear = job;
    }
    else
    {
        tm->q.front = job;
        tm->q.rear = job;
    }
    pthread_cond_signal(&tm->cond);
    tm_unlock(tm);
    debug("enqueue %d done.\n", fd);
}

server_job_t*   
server_tm_deq(server_tm_t* tm)
{
    server_job_t* job = NULL;

    debug("dequeue...\n");
    tm_lock(tm);
    pthread_cond_wait(&tm->cond, &tm->mutex);

    if (tm->q.front == NULL)
        goto unlock;

    job = tm->q.front;
    tm->q.front = tm->q.front->next;
unlock:
    tm_unlock(tm);
    debug("dequeue... done\n");
    return job;
}

void    
tm_lock(server_tm_t* tm)
{
    pthread_mutex_lock(&tm->mutex);
}

void    
tm_unlock(server_tm_t* tm)
{
    pthread_mutex_unlock(&tm->mutex);
}
