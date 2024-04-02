#include "server.h"
#include "server_events.h"

#define MAX_EP_EVENTS 10

void
server_print_sockerr(i32 fd)
{
    i32 error;
    socklen_t len = sizeof(i32);
    if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, &len) == -1)
        error("getsockopt(%d): %s\n", fd, ERRSTR);
    else
        error("socket (%d): %s\n", fd, strerror(error));
}

static void 
server_ep_event(server_t* server, const struct epoll_event* event)
{
    const i32 fd = event->data.fd;
    const u32 ev = event->events;
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
        server_print_sockerr(fd);
        server_del_event(server, se);
    }
    else if (ev & (EPOLLRDHUP | EPOLLHUP))
    {
        verbose("fd: %d hang up.\n", fd);
        server_del_event(server, se);
    }
    else if (ev & EPOLLIN)
    {
        ret = se->read(server, se);
        if (ret == SE_CLOSE || ret == SE_ERROR)
            server_del_event(server, se);
    }
    else
        warn("Not handled fd: %d, ev: 0x%x\n", fd, ev);
}

// Should ONLU be used in server_sig_handler()
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

void 
server_run(server_t* server)
{
    i32 nfds;
    struct epoll_event events[MAX_EP_EVENTS];

    server_init_signal_handlers(server);

    info("Server listening on IP: %s, port: %u\n", server->conf.addr_ip, server->conf.addr_port);

    while (server->running)
    {
        if ((nfds = epoll_wait(server->epfd, events, MAX_EP_EVENTS, -1)) == -1)
        {
            error("epoll_wait: %s\n", ERRSTR);
            server->running = false;
        }

        for (i32 i = 0; i < nfds; i++)
            server_ep_event(server, events + i);
    }
}

UNUSED static void 
server_del_all_clients(server_t* server)
{
    client_t* node = server->client_head;
    client_t* next = NULL;

    while (node)
    {
        next = node->next;
        server_free_client(server, node);
        node = next;
    }
}

static void 
server_del_all_sessions(server_t* server)
{
    session_t* node;
    session_t* next;

    node = server->session_head;

    while (node)
    {
        next = node->next;
        server_del_client_session(server, node);
        node = next;
    }
}

static void 
server_del_all_upload_tokens(server_t* server)
{
    upload_token_t* node;
    upload_token_t* next;

    node = server->upload_token_head;

    while (node)
    {
        next = node->next;
        verbose("Deleting upload token: %u for user %u\n", node->token, node->user_id);
        server_del_upload_token(server, node);
        node = next;
    }
}

void
server_del_all_events(server_t* server)
{
    server_event_t* node;
    server_event_t* next;

    node = server->se_head;

    while (node)
    {
        next = node->next;
        server_del_event(server, node);
        node = next;
    }
}

void 
server_cleanup(server_t* server)
{
    if (!server)
        return;

    server_del_all_events(server);
    server_del_all_clients(server);
    server_del_all_sessions(server);
    server_del_all_upload_tokens(server);
    server_db_close(server);
    server_close_magic(server);

    SSL_CTX_free(server->ssl_ctx);

    if (server->epfd)
        close(server->epfd);
    if (server->sock)
        close(server->sock);

    info("Server stopped.\n");

    free(server);
}

ssize_t 
server_send(const client_t* client, const void* buf, size_t len)
{
    ssize_t bytes_sent;

    if (client->secure)
        bytes_sent = SSL_write(client->ssl, buf, len);
    else
        bytes_sent = send(client->addr.sock, buf, len, MSG_NOSIGNAL);

    return bytes_sent;
}

ssize_t 
server_recv(const client_t* client, void* buf, size_t len)
{
    ssize_t bytes_recv;

    if (client->secure)
        bytes_recv = SSL_read(client->ssl, buf, len);
    else
        bytes_recv = recv(client->addr.sock, buf, len, 0);

    return bytes_recv;
}
