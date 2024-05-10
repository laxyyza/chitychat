#include "server_eworker.h"
#include "server.h"
#include "chat/db.h"

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
server_eworker_init(UNUSED eworker_t* ew)
{
    return false;
}

void 
server_eworker_async_run(UNUSED eworker_t* ew)
{

}

void 
server_eworker_cleanup(eworker_t* ew)
{
    server_db_close(&ew->db);
}
