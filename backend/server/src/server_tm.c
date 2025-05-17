#include "server_tm.h"
#include "server.h"
#include <sys/eventfd.h>

/*
 * TM - "Thread Manager"
 */

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
    server->main_ew->server = server;
    server->main_ew->db.cmd = &server->db_commands;
    strncpy(server->main_ew->name, "ew:0", THREAD_NAME_LEN);

    pthread_mutex_init(&tm->mutex, NULL);
    pthread_cond_init(&tm->cond, NULL);

    atomic_init(&tm->online_workers, 0);

    return true;
}

bool 
server_create_eworker(server_t* server, eworker_t* ew, size_t i)
{
    ew->db.cmd = &server->db_commands;
    ew->server = server;

    if (pthread_create(&ew->pth, NULL, eworker_main, ew) != 0)
    {
        fatal("pthread_create failed: %s\n", ERRSTR);
        return false;
    }
    snprintf(ew->name, THREAD_NAME_LEN, "ew:%zu", i);
    pthread_setname_np(ew->pth, ew->name);
    return true;
}

bool 
server_tm_start_threads(server_t* server)
{
    server_tm_t* tm = &server->tm;

    for (i32 i = 1; i < tm->n_workers; i++)
    {
        if (server_create_eworker(server, tm->workers + i, i) == false)
            return false;
    }

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
    eventfd_write(server->eventfd->fd, 1);

    for (i32 i = 1; i < tm->n_workers; i++)
    {
        eworker_t* ew = tm->workers + i;
        pthread_join(ew->pth, NULL);
    }
}

void
server_tm_shutdown(server_t* server)
{
    server_tm_t* tm = &server->tm;

    if (server->eventfd == NULL)
        return;

    server_tm_shutdown_threads(server);

    pthread_cond_destroy(&tm->cond);
    pthread_mutex_destroy(&tm->mutex);
}

i32
server_tm_system_threads(void)
{
    return sysconf(_SC_NPROCESSORS_ONLN);
}
