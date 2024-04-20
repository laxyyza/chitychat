#include "server.h"

i32
main(int argc, char* const* argv)
{
    server_t* server;
    i32 ret;

    /* 
     * Server Initialization: socket, epoll, OpenSSL, thread pool, etc. 
     */
    server = server_init(argc, argv);
    if (!server)
        return EXIT_FAILURE;
    
    /* 
     * Main Thread Loop: Gets events and
     * enqueus it to the thread pool.
     * `worker` main loop: server_tm.c: tm_worker()
     */
    ret = server_run(server);

    /* 
     * Cleanup: disconnecting connected clients, 
     * freeing up memory, etc.
     * And yes actually you don't *need* to free memory if 
     * your userspace process going to exit, but best practices. :)
     */
    server_cleanup(server);

    return ret;
}
