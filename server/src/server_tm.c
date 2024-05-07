#include "server_tm.h"
#include "server.h"
#include "server_evcb.h"

#define EVCB_SIZE 128

/*
 * TM - "Thread Manager"
 */

void* 
tm_worker(void* arg)
{
    server_thread_t* th = arg;
    server_t* server = th->server;
    server_tm_t* tm = &server->tm;
    ev_t ev;
    i32  new_ev;
    th->tid = gettid();

    verbose("Worker %d up and running!\n", th->tid);

    while (1)
    {
        new_ev = server_tm_deq(tm, &ev);
        if (tm->shutdown == 1)
            break;
        if (new_ev)
            server_ep_event(th, &ev);
    }

    verbose("Worker %d shutting down...\n", th->tid);
    server_db_close(&th->db);

    return NULL;
}

bool    
server_init_tm(server_t* server, i32 n_threads)
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

    server_init_evcb(server, EVCB_SIZE);
    tm->init = true;

    if (server_db_open(&server->main_th.db, server->conf.database) == false)
        return false;
    snprintf(server->main_th.name, THREAD_NAME_LEN, "main_thread");
    server->main_th.server = server;

    for (i32 i = 0; i < n_threads; i++)
        if (server_tm_init_thread(server, tm->threads + i, i) == false)
            return false;

    return true;
}

bool
server_tm_init_thread(server_t* server, server_thread_t* th, size_t i)
{
    if (server_db_open(&th->db, server->conf.database) == false)
        return false;
    th->db.cmd = &server->db_commands;
    th->server = server;

    snprintf(th->name, THREAD_NAME_LEN, "worker%zu", i);
    pthread_create(&th->pth, NULL, tm_worker, th);
    pthread_setname_np(th->pth, th->name);
    return true;
}

static void 
server_tm_shutdown_threads(server_tm_t* tm)
{
    tm_lock(tm);
    tm->shutdown = 1;

    // Clear all the events
    server_evcb_clear(&tm->cb);
    
    pthread_cond_broadcast(&tm->cond);
    tm_unlock(tm);

    for (size_t i = 0; i < tm->n_threads; i++)
        pthread_join(tm->threads[i].pth, NULL);

    server_evcb_free(&tm->cb);
}

void    
server_tm_shutdown(server_t* server)
{
    server_tm_t* tm = &server->tm;
    if (tm->init == false)
        return;

    server_tm_shutdown_threads(tm);

    pthread_cond_destroy(&tm->cond);
    pthread_mutex_destroy(&tm->mutex);

    server_db_close(&server->main_th.db);

    free(tm->threads);
}

void            
server_tm_enq(server_tm_t* tm, i32 fd, u32 ev)
{
    i32 is_full;
    do {
        tm_lock(tm);
        is_full = server_evcb_enqueue(&tm->cb, fd, ev);
        pthread_cond_broadcast(&tm->cond);
        tm_unlock(tm);
    } while (is_full);
    /**
     * If server_evcb_enqueue() returns -1, it means ring buffer is full.
     * just try to enqueue until ring buffer is open.
     **/
}

i32
server_tm_deq(server_tm_t* tm, ev_t* ev)
{
    i32 ret = -1;

    do {
        tm_lock(tm);
        if (ret == 0)
            pthread_cond_wait(&tm->cond, &tm->mutex);
        ret = server_evcb_dequeue(&tm->cb, ev);
        tm_unlock(tm);
    } while (ret == 0 && tm->shutdown == 0);

    return ret;
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
