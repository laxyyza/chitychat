#include "server.h"
#include "server_signal.h"
#include "server_tm.h"
#include <sys/eventfd.h>

void 
server_shutdown_and_notify(server_t* server, i32 return_code)
{
    bool expected = true;
    if (atomic_compare_exchange_strong(&server->running, &expected, false))
    {
        atomic_store(&server->running, false);
        server->ret = return_code;
        eventfd_write(server->eventfd, 1);
    }
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
            server_shutdown_and_notify(server, EXIT_SUCCESS);
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
server_init_signal(eworker_t* ew)
{
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGTERM);
    sigaddset(&mask, SIGPIPE);
    sigprocmask(SIG_BLOCK, &mask, NULL);
    
    ew->server->sigfd = signalfd(-1, &mask, 0);
    if (ew->server->sigfd == -1)
    {
        error("signalfd: %s\n", ERRSTR);
        return false;
    }

    add_event_args_t args = {
        .fd = ew->server->sigfd,
        .data = NULL,
        .read_cb = signal_read,
        .close_cb = signal_close,
        .write_cb = NULL,
        .name = "signalfd",
        .type = FD_SHARED
    };
    server_epoll_add_event(ew, &args);

    return true;
}
