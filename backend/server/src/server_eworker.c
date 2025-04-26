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
eworker_epoll_wait(eworker_t* ew)
{
    const server_t* server = ew->server;
    const struct epoll_event* event;
    server_event_t* se;
    i32 nfds;
    nano_timer_t timer;
    i32 log_level = server_get_loglevel();
    bool show_event_time = log_level >= SERVER_INFO && server->conf.event_time == true;

    nfds = epoll_wait(ew->epfd, ew->ep_events, EWORKER_MAX_EVENTS, -1);
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
eworker_create_socket(eworker_t* ew)
{
    i32 sock;
    server_t* server = ew->server;

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

    add_event_args_t args = {
        .fd = sock,
        .data = NULL,
        .read_cb = se_accept_conn,
        .write_cb = NULL,
        .close_cb = NULL,
        .name = "Accept Connection",
        .type = FD_EXCLUSIVE
    };
    if (server_epoll_add_event(ew, &args) == NULL)
        return false;

    return true;
}

static inline bool 
eworker_init_epoll(eworker_t* ew)
{
    ew->epfd = epoll_create1(EPOLL_CLOEXEC);
    if (ew->epfd == -1)
        fatal("%s: epoll_create1: %s\n", ew->name, ERRSTR);

    return ew->epfd != -1;
}

static enum se_status 
eworker_db_read(eworker_t* ew, UNUSED server_event_t* se)
{
    db_process_results(ew);
    return SE_OK;
}

static inline bool 
eworker_add_db_fd(eworker_t* ew)
{
    add_event_args_t args = {
        .fd = ew->db.fd,
        .data = NULL,
        .read_cb = eworker_db_read,
        .write_cb = NULL,
        .close_cb = NULL,
        .name = "PostgreSQL",
        .type = FD_EXCLUSIVE
    };
    if (server_epoll_add_event(ew, &args) == NULL)
        return false;
    return true;
}

bool 
server_eworker_init(eworker_t* ew)
{
    ew->tid = gettid();
    char* name = malloc(100);
    sprintf(name, "%s:event_ht", ew->name);

    if (!eworker_init_epoll(ew))
        return false;

    if (!server_db_open(&ew->db, &ew->server->conf, 
                        DB_PIPELINE | DB_NONBLOCK))
        return false;

    PQpipelineSync(ew->db.conn);
    db_process_results(ew);

    if (eworker_add_db_fd(ew) == false)
        return false;

    if (eworker_create_socket(ew) == false)
        return false;

    if (server_init_redis(ew) == false)
        return false;

    atomic_fetch_add(&ew->server->tm.online_workers, 1);
    debug("%s up & running!\n", ew->name);

    return true;
}

void 
server_eworker_async_run(eworker_t* ew)
{
    server_t* server = ew->server;

    while (atomic_load(&server->running))
        eworker_epoll_wait(ew);
}

void 
server_eworker_cleanup(eworker_t* ew)
{
    server_db_close(&ew->db);
    server_deinit_redis(ew);
    close(ew->epfd);
    debug("%s shutdown.\n", ew->name);
}

