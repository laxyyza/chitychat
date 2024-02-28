#include "server.h"

int main(int argc, const char** argv)
{
    server_t* server = server_init(SERVER_CONFIG_PATH);
    if (!server)
        return -1;

    server_run(server);

    server_cleanup(server);

    return 0;;
}