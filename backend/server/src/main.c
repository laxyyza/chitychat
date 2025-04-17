#include "server.h"

i32
main(int argc, char* const* argv)
{
    server_t* server;

    server = server_init(argc, argv);
    if (!server)
        return EXIT_FAILURE;

    server_run(server);

    server_cleanup(server);

    return EXIT_SUCCESS;
}
