#include "server_eworker.h"
#include "server.h"
#include "chat/db.h"
#include "chat/db_pipeline.h"
#include "server_tm.h"
#include <poll.h>

static void
eworker_process_results(eworker_t* ew)
{
    // TODO
}

static void*
eworker_main(void* arg)
{
    if (server_eworker_init(arg) == false)
        return NULL;
    server_eworker_async_run(arg);
    server_eworker_cleanup(arg);
    return NULL;
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

    debug("%s up & running!\n", ew->name);
    return true;
}

void 
server_eworker_async_run(eworker_t* ew)
{
    server_t* server = ew->server;
    server_tm_t* tm = &server->tm;
    server_db_t* db = &ew->db;
    struct pollfd pfd = {
        .fd = ew->db.fd,
        .events = POLLIN
    };
    i32 ret;
    i32 deq_flag;
    ev_t ev;

    while ((tm->state & TM_STATE_SHUTDOWN) == 0)
    {
        if ((ret = poll(&pfd, 1, 0)) == -1) 
        {
            error("poll");
            continue;
        }

        if (ret > 0)
            eworker_process_results(ew);

        /*
         * Block if no async jobs
         * Don't Block if async jobs in queue
         */
        deq_flag = (db->queue.count == 0) ? TM_DEQ_BLOCK : TM_DEQ_NONBLOCK;
        
        /* Dequeue Event */
        if (server_tm_deq(tm, &ev, deq_flag))
        {
            // Set ew current job 
            // Handle event
            server_ep_event(ew, &ev);

            // After event check if any sql queries
            // if so enqueue it to ew->queue
            db_pipeline_current_done(db);
        }
    }
}

void 
server_eworker_cleanup(eworker_t* ew)
{
    server_db_close(&ew->db);
    debug("%s shutdown.\n", ew->name);
}
