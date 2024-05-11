#include "server_tm.h"
#include "chat/db_def.h"
#include "server.h"
#include <sys/eventfd.h>

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

    tm->state |= TM_STATE_INIT;

    if (server_db_open(&server->main_ew.db, server->conf.database, 0) == false)
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
server_tm_shutdown_threads(server_t* server)
{
    server_tm_t* tm = &server->tm;
    tm->state |= TM_STATE_SHUTDOWN;

    /*
     * If workers are not busy, they will block in epoll_wait()
     * to wake them up from that, write something to eventfd.
     */
    eventfd_write(server->eventfd, 1);
    
    for (size_t i = 0; i < tm->n_workers; i++)
    {
        eworker_t* ew = tm->workers + i;
        pthread_join(ew->pth, NULL);
    }
}

void    
server_tm_shutdown(server_t* server)
{
    server_tm_t* tm = &server->tm;
    if (!(tm->state & TM_STATE_INIT))
        return;

    if (server->eventfd == 0)
        return;

    server_tm_shutdown_threads(server);

    pthread_cond_destroy(&tm->cond);
    pthread_mutex_destroy(&tm->mutex);

    server_db_close(&server->main_ew.db);

    free(tm->workers);
}

// void    
// tm_lock(server_tm_t* tm)
// {
//     pthread_mutex_lock(&tm->mutex);
// }
//
// void    
// tm_unlock(server_tm_t* tm)
// {
//     pthread_mutex_unlock(&tm->mutex);
// }

i32     
server_tm_system_threads(void)
{
    return sysconf(_SC_NPROCESSORS_ONLN);
}
