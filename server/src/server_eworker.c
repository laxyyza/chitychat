#include "server_eworker.h"
#include "chat/db.h"

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
