#include "server.h"
#include "server_signal.h"
#include "server_tm.h"
#include <sys/eventfd.h>

static void 
shutdown_and_notify(server_t* server)
{
    server->running = false;
    eventfd_write(server->eventfd, 1);
}

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
            shutdown_and_notify(server);
            break;
        default:
            break;
    }
}

static enum se_status
signal_read(eworker_t* ew, server_event_t* ev)
{
    struct signalfd_siginfo siginfo;
    const ssize_t size = sizeof(struct signalfd_siginfo);

    if (read(ev->fd, &siginfo, size) != size)
    {
        fatal("read signalfd: %s\n", ERRSTR);
        ew->server->running = false;
        return SE_ERROR;
    }
    server_handle_signal(ew->server, &siginfo);

    return SE_OK;
}

static enum se_status
signal_close(eworker_t* ew, server_event_t* ev)
{
    close(ev->fd);
    if (ew->server->running)
        ew->server->running = false;
    return SE_OK;
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

    server_new_event(server, 
                     server->sigfd, 
                     NULL, 
                     signal_read, 
                     signal_close);

    return true;
}
