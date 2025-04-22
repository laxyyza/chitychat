#include "server_eworker.h"
#include "db/db.h"
#include "db/db_def.h"
#include "db/db_pipeline.h"
#include "server_events.h"
#include "server_tm.h"
#include "server.h"
#include <libpq-fe.h>
#include <netinet/tcp.h>
#include "nano_timer.h"

#define LISTEN_BACKLOG 100

void*
eworker_main(void* arg)
{
    if (server_eworker_init(arg) == false)
        return NULL;
    server_eworker_async_run(arg);
    server_eworker_cleanup(arg);
    return NULL;
}

static void
eworker_prep_event(eworker_t* ew, server_event_t* se)
{
    server_process_event(ew, se);
    db_pipeline_current_done(&ew->db);
}

static void 
eworker_print_event_time(eworker_t* ew, i64 time_elpased_ns, server_event_t* se)
{
    f64 time_elpased_ms = time_elpased_ns / 1e6;
    f64 time_elpased_us = time_elpased_ns / 1e3;

    info("EVENT_TIME: %s: ", ew->name);
    if (strlen(ew->name) < 5)
        printf(" ");
    if (time_elpased_ms >= 1.0)
        printf("%.2f ms", time_elpased_ms);
    else if (time_elpased_us >= 1.0)
        printf("%.2f µs", time_elpased_us);
    else
        printf("%ld ns", time_elpased_ns);
    printf("\t(%s)\n", se->debug_name);
}

static inline void 
eworker_wait_for_events(eworker_t* ew)
{
    const server_t* server = ew->server;
    const struct epoll_event* event;
    server_event_t* se;
    i32 nfds;
    i32 timeout;
    nano_timer_t timer;
    i32 log_level = server_get_loglevel();
    bool show_event_time = log_level >= SERVER_INFO && server->conf.event_time == true;

    /* Block if pipeline is empty, else return immediately. */
    timeout = (ew->db.queue.count == 0 && ew->redis.cmds == 0) ? -1 : 0;

    nfds = epoll_wait(server->epfd, ew->ep_events, EWORKER_MAX_EVENTS, timeout);
    if (nfds == -1)
    {
        error("%s: epoll_wait: %s",
              ew->name, ERRSTR);
        return;
    }

    for (i32 i = 0; i < nfds; i++)
    {
        event = ew->ep_events + i;
        se = event->data.ptr;
        se->ep_events = event->events;

        if (show_event_time)
            nano_start_time(&timer);

        eworker_prep_event(ew, se);

        if (show_event_time)
        {
            i64 time_elpased_ns = nano_end_time(&timer);
            eworker_print_event_time(ew, time_elpased_ns, se);
        }
    }
}

static bool 
eworker_create_socket(server_t* server)
{
    i32 sock;

    sock = socket(server->domain, SOCK_STREAM, 0);
    if (sock == -1)
    {
        fatal("socket: %s\n", strerror(errno));
        return false;
    }

    int opt = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, server->addr_len) == -1)
        error("setsockopt: %s\n", strerror(errno));

    opt = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(i32)) == -1)
        error("setsockopt: %s\n", strerror(errno));

    opt = 1;
    if (setsockopt(sock, SOL_TCP, TCP_NODELAY, &opt, sizeof(i32)) == -1)
        error("setsockopt: %s\n", strerror(errno));

    if (bind(sock, server->addr, server->addr_len) == -1)
    {   
        fatal("bind: %s\n", strerror(errno));
        return false;
    }

    if (listen(sock, LISTEN_BACKLOG) == -1)
    {
        fatal("listen: %s\n", strerror(errno)); 
        return false;
    }

    if (server_new_event(server, sock, NULL, se_accept_conn, NULL, "Accpet Connection") == NULL)
        return false;

    return true;
}

bool 
server_eworker_init(eworker_t* ew)
{
    ew->tid = gettid();

    if (!server_db_open(&ew->db, &ew->server->conf, 
                        DB_PIPELINE | DB_NONBLOCK))
        return false;

    PQpipelineSync(ew->db.conn);
    db_process_results(ew);

    ew->pfds[0].fd = ew->db.fd;
    ew->pfds[0].events = POLLIN;

    if (eworker_create_socket(ew->server) == false)
        return false;

    if (server_init_redis(ew) == false)
        return false;

    debug("%s up & running!\n", ew->name);
    return true;
}

static inline void 
eworker_db_poll(eworker_t* ew)
{
    i32 ret;

    if ((ret = poll(ew->pfds, PFDS_COUNT, 0)) == -1) 
    {
        error("poll: %s\n", ERRSTR);
        ew->server->running = false;
    }

    if (ret > 0)
    {
        if (ew->pfds[0].revents & POLLIN)
            db_process_results(ew);

        if (ew->pfds[1].revents & POLLIN)
            redisAsyncHandleRead(ew->redis.c);
        if (ew->pfds[1].revents & POLLOUT)
            redisAsyncHandleWrite(ew->redis.c);
    }
}

void 
server_eworker_async_run(eworker_t* ew)
{
    server_t* server = ew->server;

    while (server->running)
    {
        // poll() for thread-specific events.
        eworker_db_poll(ew);

        // epoll() for general events.
        eworker_wait_for_events(ew);
    }
}

void 
server_eworker_cleanup(eworker_t* ew)
{
    server_db_close(&ew->db);
    debug("%s shutdown.\n", ew->name);
}

