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
    if (server_eworker_init(arg) == false)
        return NULL;
    server_eworker_async_run(arg);
    server_eworker_cleanup(arg);
    return NULL;

    // eworker_t* th = arg;
    // server_t* server = th->server;
    // server_tm_t* tm = &server->tm;
    // ev_t ev;
    // i32  new_ev;
    // th->tid = gettid();
    //
    // verbose("Worker %d up and running!\n", th->tid);
    //
    // while (1)
    // {
    //     new_ev = server_tm_deq(tm, &ev);
    //     if (tm->shutdown == 1)
    //         break;
    //     if (new_ev)
    //         server_ep_event(th, &ev);
    // }
    //
    // verbose("Worker %d shutting down...\n", th->tid);
    // server_db_close(&th->db);
    //
}

bool    
server_init_tm(server_t* server, i32 n_workers)
{
    server_tm_t* tm = &server->tm;

    if (n_workers <= 0)
    {
        fatal("n_threads (%d) <= 0\n", n_workers);
        return false;
    }

    tm->n_workers = n_workers;
    tm->workers = calloc(n_workers, sizeof(eworker_t));

    pthread_mutex_init(&tm->mutex, NULL);
    pthread_cond_init(&tm->cond, NULL);

    server_init_evcb(server, EVCB_SIZE);
    tm->state |= TM_STATE_INIT;

    if (server_db_open(&server->main_ew.db, server->conf.database) == false)
        goto err;
    snprintf(server->main_ew.name, THREAD_NAME_LEN, "main_thread");
    server->main_ew.server = server;

    for (i32 i = 0; i < n_workers; i++)
        if (server_create_eworker(server, tm->workers + i, i) == false)
            goto err;
    return true;
err:
    tm->state |= TM_STATE_SHUTDOWN;
    return false;
}

static void 
server_tm_shutdown_threads(server_tm_t* tm)
{
    tm_lock(tm);
    tm->state |= TM_STATE_SHUTDOWN;

    // Clear all the events
    server_evcb_clear(&tm->eq);
    
    pthread_cond_broadcast(&tm->cond);
    tm_unlock(tm);

    for (size_t i = 0; i < tm->n_workers; i++)
        pthread_join(tm->workers[i].pth, NULL);

    server_evcb_free(&tm->eq);
}

void    
server_tm_shutdown(server_t* server)
{
    server_tm_t* tm = &server->tm;
    if (!(tm->state & TM_STATE_INIT))
        return;

    server_tm_shutdown_threads(tm);

    pthread_cond_destroy(&tm->cond);
    pthread_mutex_destroy(&tm->mutex);

    server_db_close(&server->main_ew.db);

    free(tm->workers);
}

void            
server_tm_enq(server_tm_t* tm, i32 fd, u32 ev)
{
    i32 is_full;
    do {
        tm_lock(tm);
        is_full = server_evcb_enqueue(&tm->eq, fd, ev);
        pthread_cond_broadcast(&tm->cond);
        tm_unlock(tm);
    } while (is_full);
    /**
     * If server_evcb_enqueue() returns -1, it means ring buffer is full.
     * just try to enqueue until ring buffer is open.
     **/
}

i32
server_tm_deq(server_tm_t* tm, ev_t* ev, i32 flags)
{
    i32 ret = -1;

    do {
        if (flags == TM_DEQ_BLOCK)
            pthread_mutex_lock(&tm->mutex);
        else if (pthread_mutex_trylock(&tm->mutex) != 0)
            return 0;
        pthread_cond_wait(&tm->cond, &tm->mutex);
        ret = server_evcb_dequeue(&tm->eq, ev);
        pthread_mutex_unlock(&tm->mutex);
    } while (ret == 0 && !(tm->state & TM_STATE_SHUTDOWN));

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
