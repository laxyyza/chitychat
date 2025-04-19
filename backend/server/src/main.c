#include "server.h"
#include "nano_timer.h"

i32
main(int argc, char* const* argv)
{
    server_t* server;
    nano_timer_t timer;
    i64 time_elapsed_ns;

    nano_start_time(&timer);

    server = server_init(argc, argv);
    if (!server)
        return EXIT_FAILURE;

    time_elapsed_ns = nano_end_time(&timer);
    info("server_init(): %.2fms\n", 
         (time_elapsed_ns / 1e6));

    server_run(server);

    server_cleanup(server);

    return EXIT_SUCCESS;
}
