#include "server.h"
#include "server_client.h"

i32
server_print_sockerr(i32 fd)
{
    i32 error;
    socklen_t len = sizeof(i32);
    if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, &len) == -1)
        error("getsockopt(%d): %s\n", fd, ERRSTR);
    else
        error("socket (%d): %s\n", fd, strerror(error));

    return error;
}

void 
server_ep_event(server_thread_t* th, const ev_t* job)
{
    server_t* server = th->server;
    const i32 fd = job->fd;
    const u32 ev = job->ev;
    enum se_status ret;
    server_event_t* se = server_get_event(server, fd);

    if (!se)
    {
        error("server_get_event(%d) failed!\n", fd);
        server_ep_delfd(server, fd);
        return;
    }
    se->ep_events = ev;

    if (ev & EPOLLERR)
    {
        se->err = server_print_sockerr(fd);
        server_del_event(th, se);
    }
    else if (ev & (EPOLLRDHUP | EPOLLHUP))
    {
        verbose("fd: %d hang up.\n", fd);
        server_del_event(th, se);
    }
    else if (ev & EPOLLIN)
    {
        ret = se->read(th, se);
        if (ret == SE_CLOSE || ret == SE_ERROR)
            server_del_event(th, se);
        else if (se->listen_events & EPOLLONESHOT)
            server_ep_rearm(server, fd);
    }
    else
        warn("Not handled fd: %d, ev: 0x%x\n", fd, ev);
}

// Should ONLY be used in server_sig_handler()
server_t* __server_sig = NULL;

static void 
server_sig_handler(i32 signum)
{
    debug("signal: %d\n", signum);

    if (signum == SIGINT || signum == SIGTERM)
        __server_sig->running = false;
}

static void 
server_init_signal_handlers(server_t* server)
{
    struct sigaction sa;
    sa.sa_handler = server_sig_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    __server_sig = server;

    if (sigaction(SIGINT, &sa, NULL) == -1)
        error("sigaction: %s\n", ERRSTR);
    if (sigaction(SIGPIPE, &sa, NULL) == -1)
        error("sigaction: %s\n", ERRSTR);
    if (sigaction(SIGTERM, &sa, NULL) == -1)
        error("sigaction: %s\n", ERRSTR);
}

i32
server_run(server_t* server)
{
    i32 ret = EXIT_SUCCESS;
    i32 nfds;
    const struct epoll_event* epev;

    server_init_signal_handlers(server);

    info("Server listening on IP: %s, port: %u, thread pool: %zu\n", 
         server->conf.addr_ip, server->conf.addr_port, server->tm.n_threads);

    while (server->running)
    {
        if ((nfds = epoll_wait(server->epfd, server->ep_events, MAX_EP_EVENTS, -1)) == -1)
        {
            if (errno == EINTR)
                continue;

            error("epoll_wait: %s\n", ERRSTR);
            server->running = false;
            ret = EXIT_FAILURE;
        }

        for (i32 i = 0; i < nfds; i++)
        {
            epev = server->ep_events + i;
            server_tm_enq(&server->tm, epev->data.fd, epev->events);
        }
    }

    return ret;
}

static void 
server_del_all_clients(server_t* server)
{
    server_ght_t* ht = &server->client_ht;
    ht->ignore_resize = true;

    GHT_FOREACH(client_t* client, ht, {
        server_free_client(&server->main_th, client);
    });
    server_ght_destroy(ht);
}

static void 
server_del_all_sessions(server_t* server)
{
    server_ght_t* ht = &server->session_ht;
    ht->ignore_resize = true;

    GHT_FOREACH(session_t* session, ht, {
        server_del_user_session(server, session);
    });
    server_ght_destroy(ht);
}

static void 
server_del_all_upload_tokens(server_t* server)
{
    server_ght_t* ht = &server->upload_token_ht;
    ht->ignore_resize = true;

    GHT_FOREACH(upload_token_t* ut, ht, {
        server_del_upload_token(&server->main_th, ut);
    });
    server_ght_destroy(ht);
}

void
server_del_all_events(server_t* server)
{
    server_ght_t* ht = &server->event_ht;
    ht->ignore_resize = true;

    GHT_FOREACH(server_event_t* ev, ht, {
        server_del_event(&server->main_th, ev);
    });
    server_ght_destroy(&server->event_ht);
}

void 
server_cleanup(server_t* server)
{
    if (!server)
        return;

    server_tm_shutdown(server);
    server_ght_destroy(&server->chat_cmd_ht);
    server_del_all_events(server);
    server_del_all_clients(server);
    server_del_all_sessions(server);
    server_del_all_upload_tokens(server);
    server_db_free(server);
    server_close_magic(server);

    SSL_CTX_free(server->ssl_ctx);

    if (server->epfd)
        close(server->epfd);
    if (server->sock)
        close(server->sock);

    debug("Server stopped.\n");

    free(server);
}

static void
server_print_ssl_error(client_t* client, i32 ret, const char* from)
{
    i32 err = SSL_get_error(client->ssl, ret);
    if (err == SSL_ERROR_NONE)
        return;
    error("SSL %s: %s\n", from, ERR_error_string(err, NULL));
}

ssize_t 
server_send(client_t* client, const void* buf, size_t len)
{
    ssize_t bytes_sent;

    if (client->err == CLIENT_ERR_SSL)
        return -1;

    bytes_sent = SSL_write(client->ssl, buf, len);
    if (bytes_sent <= 0)
    {
        server_print_ssl_error(client, bytes_sent, "write");
        server_set_client_err(client, CLIENT_ERR_SSL);
    }

    return bytes_sent;
}

ssize_t 
server_recv(client_t* client, void* buf, size_t len)
{
    ssize_t bytes_recv;

    if (client->err == CLIENT_ERR_SSL)
        return -1;

    bytes_recv = SSL_read(client->ssl, buf, len);
    if (bytes_recv <= 0)
    {
        server_print_ssl_error(client, bytes_recv, "read");
        server_set_client_err(client, CLIENT_ERR_SSL);
    }

    return bytes_recv;
}
