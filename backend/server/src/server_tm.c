#include "server_tm.h"
#include "chat/db_def.h"
#include "server.h"
#include <sys/eventfd.h>

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
    server->main_ew = &tm->workers[0];

    pthread_mutex_init(&tm->mutex, NULL);
    pthread_cond_init(&tm->cond, NULL);

    for (i32 i = 1; i < n_workers; i++)
        if (server_create_eworker(server, tm->workers + i, i) == false)
            return false;
    return true;
}

static void 
server_tm_shutdown_threads(server_t* server)
{
    server_tm_t* tm = &server->tm;

    /*
     * If workers are not busy, they will block in epoll_wait()
     * to wake them up from that, write something to eventfd.
     */
    eventfd_write(server->eventfd, 1);
    
    for (size_t i = 1; i < tm->n_workers; i++)
    {
        eworker_t* ew = tm->workers + i;
        pthread_join(ew->pth, NULL);
    }
}

void
server_tm_shutdown(server_t* server)
{
    server_tm_t* tm = &server->tm;

    if (server->eventfd <= 0)
        return;

    server->running = false;

    server_tm_shutdown_threads(server);

    pthread_cond_destroy(&tm->cond);
    pthread_mutex_destroy(&tm->mutex);

    free(tm->workers);
}

i32
server_tm_system_threads(void)
{
    return sysconf(_SC_NPROCESSORS_ONLN);
}
