#include "server_eworker.h"
#include "chat/db.h"
#include "chat/db_def.h"
#include "chat/db_pipeline.h"
#include "server_events.h"
#include "server_tm.h"
#include "server.h"
#include <libpq-fe.h>
#include <poll.h>

static void*
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
eworker_wait_for_events(eworker_t* ew)
{
    const server_t* server = ew->server;
    const struct epoll_event* event;
    server_event_t* se;
    i32 nfds;
    i32 timeout;

    /* Block if pipeline is empty, else return immediately. */
    timeout = (ew->db.queue.count == 0) ? -1 : 0;

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

        eworker_prep_event(ew, se);
    }
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
server_eworker_init(eworker_t* ew)
{
    ew->tid = gettid();
    if (!server_db_open(&ew->db, ew->server->conf.database, 
                        DB_PIPELINE | DB_NONBLOCK))
        return false;

    PQpipelineSync(ew->db.conn);
    db_process_results(ew);

    debug("%s up & running!\n", ew->name);
    return true;
}

void 
server_eworker_async_run(eworker_t* ew)
{
    server_t* server = ew->server;
    server_tm_t* tm = &server->tm;
    // server_db_t* db = &ew->db;
    struct pollfd pfd = {
        .fd = ew->db.fd,
        .events = POLLIN
    };
    i32 ret;

    while ((tm->state & TM_STATE_SHUTDOWN) == 0)
    {
        if ((ret = poll(&pfd, 1, 0)) == -1) 
        {
            error("poll: %s\n", ERRSTR);
            tm->state |= TM_STATE_SHUTDOWN;
            break;
        }

        // debug("tid: %d\n\tret: %d\n\tdb->queue: %d\n", 
        //       ew->tid, ret, db->queue.count);

        if (ret > 0)
            db_process_results(ew);

        eworker_wait_for_events(ew);
    }
}

void 
server_eworker_cleanup(eworker_t* ew)
{
    server_db_close(&ew->db);
    debug("%s shutdown.\n", ew->name);
}

