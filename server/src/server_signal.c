#include "server.h"
#include "server_events.h"
#include "server_signal.h"

enum se_status
server_handle_signal(eworker_t* th, server_event_t* ev)
{
    struct signalfd_siginfo siginfo;
    server_signal_t* sig_hlr = ev->data;
    i32 sig;

    if (read(sig_hlr->fd, &siginfo, sizeof(struct signalfd_siginfo)) == -1)
    {
        error("read signalfd: %s\n", ERRSTR);
        return SE_ERROR;
    }

    sig = siginfo.ssi_signo;

    debug("Signaled: %d (%s)\n", sig, strsignal(sig));

    switch (sig)
    {
        case SIGINT:
        case SIGTERM:
        {
            th->server->running = false;
            kill(th->server->pid, SIGUSR1); // Interrupt main thread
            break;
        }
        default:
            break;
    }

    return SE_OK;
}

bool 
server_init_signal(server_t* server)
{
    server_signal_t* sig = &server->sig_hlr;

    sigemptyset(&sig->mask);
    sigaddset(&sig->mask, SIGINT);
    sigaddset(&sig->mask, SIGTERM);
    sigaddset(&sig->mask, SIGPIPE);
    sigaddset(&sig->mask, SIGUSR1);
    sigprocmask(SIG_BLOCK, &sig->mask, NULL);
    
    sig->fd = signalfd(-1, &sig->mask, 0);
    if (sig->fd == -1)
    {
        error("signalfd: %s\n", ERRSTR);
        return false;
    }

    server_new_event(server, sig->fd, sig, 
                     server_handle_signal, server_close_signal);

    return true;
}

enum se_status
server_close_signal(UNUSED eworker_t* th, server_event_t* ev)
{
    server_signal_t* sig = ev->data;
    close(sig->fd);
    return SE_OK;
}
