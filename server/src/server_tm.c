#include "server_tm.h"
#include "server.h"
#include <pthread.h>

void* 
tm_worker(void* arg)
{
    server_thread_t* th = arg;
    server_t* server = th->server;
    server_tm_t* tm = &server->tm;
    server_job_t* job;
    th->tid = gettid();

    verbose("Worker %d up and running!\n", th->tid);

    while (1)
    {
        job = server_tm_deq(tm);

        if (tm->shutdown == 1)
            break;

        if (job)
        {
            server_ep_event(th, job);
            free(job);
            job = NULL;
        }
    }

    verbose("Worker %d shutting down...\n", th->tid);

    return NULL;
}

bool    
server_tm_init(server_t* server, i32 n_threads)
{
    server_tm_t* tm = &server->tm;

    if (n_threads <= 0)
    {
        fatal("n_threads (%d) <= 0\n", n_threads);
        return false;
    }

    tm->n_threads = n_threads;
    tm->threads = calloc(n_threads, sizeof(server_thread_t));
    tm->shutdown = 0;

    pthread_mutex_init(&tm->mutex, NULL);
    pthread_cond_init(&tm->cond, NULL);

    for (i32 i = 0; i < n_threads; i++)
        if (server_tm_init_thread(server, tm->threads + i, i) == false)
            return false;

    return true;
}

bool
server_tm_init_thread(server_t* server, server_thread_t* th, size_t i)
{
    if (server_db_open(&th->db) == false)
        return false;
    th->db.cmd = &server->db_commands;
    th->server = server;

    snprintf(th->name, THREAD_NAME_LEN, "worker%zu", i);
    pthread_create(&th->pth, NULL, tm_worker, th);
    pthread_setname_np(th->pth, th->name);
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
        server_db_close(&tm->threads[i].db);

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
}

server_job_t*   
server_tm_deq(server_tm_t* tm)
{
    server_job_t* job = NULL;

    tm_lock(tm);
    pthread_cond_wait(&tm->cond, &tm->mutex);

    if (tm->q.front == NULL)
        goto unlock;

    job = tm->q.front;
    tm->q.front = tm->q.front->next;
unlock:
    tm_unlock(tm);
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

i32     
server_tm_system_threads(void)
{
    return sysconf(_SC_NPROCESSORS_ONLN);
}
