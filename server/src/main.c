#include "server.h"

int 
main(int argc, char* const* argv)
{
    server_t* server = server_init(argc, argv);
    if (!server)
        return -1;

    if (server->conf.fork)
    {
        if (fork() != 0)
            return 0;
    }
    
    server_run(server);

    server_cleanup(server);

    return 0;;
}