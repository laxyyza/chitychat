#include "server.h"
#include "server_signal.h"

static void
server_handle_signal(server_t* server, const struct signalfd_siginfo* siginfo)
{
    i32 sig;

    sig = siginfo->ssi_signo;

    debug("Signaled: %d (%s)\n", sig, strsignal(sig));

    switch (sig)
    {
        case SIGINT:
        case SIGTERM:
            server->running = false;
            break;
        default:
            break;
    }
}

bool 
server_init_signal(server_t* server)
{
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGTERM);
    sigaddset(&mask, SIGPIPE);
    sigprocmask(SIG_BLOCK, &mask, NULL);
    
    server->sigfd = signalfd(-1, &mask, 0);
    if (server->sigfd == -1)
    {
        error("signalfd: %s\n", ERRSTR);
        return false;
    }

    return true;
}

void 
server_wait_for_signals(server_t* server)
{
    struct signalfd_siginfo siginfo;
    const ssize_t size = sizeof(struct signalfd_siginfo);

    while (server->running)
    {
        if (read(server->sigfd, &siginfo, size) != size)
        {
            fatal("read signalfd: %s\n", ERRSTR);
            server->running = false;
            return;
        }
        server_handle_signal(server, &siginfo);
    }
}
