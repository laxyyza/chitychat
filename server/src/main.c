#include "server.h"

i32
main(int argc, char* const* argv)
{
    server_t* server;

    /* 
     * Server Initialization: socket, epoll, OpenSSL, thread pool, etc. 
     */
    server = server_init(argc, argv);
    if (!server)
        return EXIT_FAILURE;
    
    /* 
     * Main thread only handle signals.
     */
    server_run(server);

    /* 
     * Cleanup: disconnecting connected clients, 
     * freeing up memory, etc.
     * And yes actually you don't *need* to free memory if 
     * your userspace process going to exit, but best practices. :)
     */
    server_cleanup(server);

    return EXIT_SUCCESS;
}
