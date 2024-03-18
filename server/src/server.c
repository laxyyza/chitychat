#include "server.h"

#define MAX_EP_EVENTS 10

i32 
server_ep_addfd(server_t* server, i32 fd)
{
    i32 ret;

    struct epoll_event ev = {
        .data.fd = fd,
        .events = EPOLLIN | EPOLLRDHUP | EPOLLHUP
    };

    ret = epoll_ctl(server->epfd, EPOLL_CTL_ADD, fd, &ev);
    if (ret == -1)
        error("epoll_ctl ADD fd:%d: %s\n", fd, ERRSTR);

    return ret;
}

i32 
server_ep_delfd(server_t* server, i32 fd)
{
    i32 ret;

    if (!server || !fd)
        return -1;

    ret = epoll_ctl(server->epfd, EPOLL_CTL_DEL, fd, NULL);
    if (ret == -1 && errno != ENOENT)
        error("epoll_ctl DEL fd:%d: %s\n", fd, ERRSTR);

    return ret;
}

static void 
server_get_client_info(client_t* client)
{
    i32 ret;
    i32 domain;

    ret = getnameinfo(client->addr.addr_ptr, client->addr.len, client->addr.host, NI_MAXHOST, client->addr.serv, NI_MAXSERV, NI_NUMERICHOST | NI_NUMERICSERV);
    if (ret == -1)
    {
        error("getnameinfo: %s\n", ERRSTR);
    }

    if (client->addr.version == IPv4)
    {
        domain = AF_INET;
        inet_ntop(domain, &client->addr.ipv4.sin_addr, client->addr.ip_str, INET_ADDRSTRLEN);
    }
    else
    {
        domain = AF_INET6;
        inet_ntop(domain, &client->addr.ipv6.sin6_addr, client->addr.ip_str, INET6_ADDRSTRLEN);
    }

}

static void 
server_accept_conn(server_t* server)
{
    i32 ret;
    client_t* client = server_new_client(server);

    client->addr.len = server->addr_len;
    client->addr.version = server->conf.addr_version;
    client->addr.addr_ptr = (struct sockaddr*)&client->addr.ipv4;
    client->addr.sock = accept(server->sock, client->addr.addr_ptr, &client->addr.len);
    if (client->addr.sock == -1)
    {
        error("accept: %s\n", ERRSTR);
        return;
    }

    ret = server_client_ssl_handsake(server, client);

    server_get_client_info(client);

    if (ret == -1 || (ret == 0 && server->conf.secure_only))
    {
        verbose("Client (fd:%d, IP: %s:%s) SSL handsake failed.\n",
            client->addr.sock, client->addr.ip_str, client->addr.serv);
        server_del_client(server, client);
        return;
    }

    server_ep_addfd(server, client->addr.sock);

    info("Client (fd:%d, IP: %s:%s) connected.\n", 
        client->addr.sock, client->addr.ip_str, client->addr.serv);
}

static void 
server_read_fd(server_t* server, const i32 fd)
{
    ssize_t bytes_recv;
    u8* buf;
    size_t buf_size;
    size_t offset = 0;
    http_t* http;
    enum client_recv_status recv_status = RECV_OK;
    client_t* client = server_get_client_fd(server, fd);
    if (!client)
    {
        // TODO: Fix missing clients
        error("fd: %d no client. FIX YOUR LINKED LIST!\n", fd);
        server_ep_delfd(server, fd);
        return;
    }

    http = client->recv.http;

    if (http)
    {
        buf = (u8*)http->body + http->buf.total_recv;
        buf_size = http->body_len - http->buf.total_recv;
    }
    else
    {
        if (!client->recv.data)
        {
            client->recv.data = calloc(1, CLIENT_RECV_PAGE);
            client->recv.data_size = CLIENT_RECV_PAGE - 1;
        }
        else
            offset = client->recv.offset;
        buf = client->recv.data;
        buf_size = client->recv.data_size;
        // memset(buf, 0, buf_size);
    }

    bytes_recv = server_recv(client, buf + offset, buf_size - offset);
    if (bytes_recv == -1)
    {
        client->recv.n_errors++;
        error("Client (fd:%d, IP: %s:%s) recv #%d error: %s\n", 
            client->addr.sock, client->addr.ip_str, client->recv.n_errors, client->addr.serv);
        if (client->recv.n_errors >= CLIENT_MAX_ERRORS)
            server_del_client(server, client);
        return;
    }
    else if (bytes_recv == 0)
    {
        verbose("Client recv 0 bytes, disconnecting...\n");
        server_del_client(server, client);
        return;
    }
    else if (http)
    {
        http->buf.total_recv += bytes_recv;
        verbose("HTTP recv: %zu/%zu\n", http->buf.total_recv, http->body_len);
        if ((size_t)bytes_recv >= buf_size)
        {
            server_handle_http(server, client, client->recv.http);
            client->recv.http = NULL;
        }
    }
    else
    {
        if (client->state & CLIENT_STATE_WEBSOCKET)
            recv_status = server_ws_parse(server, client, buf, bytes_recv + offset);
        else
            recv_status = server_http_parse(server, client, buf, bytes_recv);
    }

    if (recv_status != RECV_DISCONNECT && !client->recv.busy)
    {
        free(client->recv.data);
        client->recv.data = NULL;
        client->recv.data_size = 0;
        client->recv.offset = 0;
    }

    if (recv_status == RECV_OK && client->recv.overflow_check != CLIENT_OVERFLOW_CHECK_MAGIC)
    {
        error("Client fd: %d recv overflow check failed! %X != %X\n", client->addr.sock, client->recv.overflow_check, CLIENT_OVERFLOW_CHECK_MAGIC);
    }
}

static void 
server_ep_event(server_t* server, const struct epoll_event* event)
{
    const i32 fd = event->data.fd;
    const u32 ev = event->events;

    if (ev & EPOLLERR)
    {
        i32 error = 0;
        socklen_t errlen = sizeof(i32);
        if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, &errlen) == 0)
        {
            error("Epoll error on fd: %d: %s\n", fd, strerror(error));
        }
        else
            error("epoll error on fd: %d, (no error string).\n", fd);
    }

    if (ev & EPOLLRDHUP || ev & EPOLLHUP)
    {
        client_t* client = server_get_client_fd(server, fd);
        verbose("fd: %d hang up.\n", fd);
        if (client)
            server_del_client(server, client);
        else
        {
            warn("fd: %d no client, removing from epoll.\n", fd);
            server_ep_delfd(server, fd);
            close(fd);
        }

        return;
    }
    
    if (fd == server->sock)
    {
        // New connection...
        server_accept_conn(server);
    }
    else if (ev & EPOLLIN)
    {
        // fd ready for read
        server_read_fd(server, fd);
    }
    else
    {
        warn("Not handled fd: %d, ev: %u\n", fd, ev);
    }
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

static void 
server_del_all_clients(server_t* server)
{
    client_t* node = server->client_head;
    client_t* next = NULL;

    while (node)
    {
        next = node->next;
        server_del_client(server, node);
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
server_cleanup(server_t* server)
{
    if (!server)
        return;

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
 
server_event_t* 
server_new_event(server_t* server, i32 fd, void* data, 
            se_read_callback_t read_callback, se_close_callback_t close_callback)
{
    server_event_t* se;
    server_event_t* head_next;

    if (!server || !data || !read_callback || !close_callback)
    {
        warn("server_new_event(%p, %d, %p, %p, %p): Something is NULL!\n",
                server, fd, data, read_callback, close_callback);
        return NULL;
    }
    
    se = calloc(1, sizeof(server_event_t));
    if (server_ep_addfd(server, fd) == -1)
    {
        free(se);
        error("ep_addfd %d failed\n", fd);
        return NULL;
    }
    se->fd = fd;
    se->data = data;
    se->read = read_callback;
    se->close = close_callback;

    if (server->se_head == NULL)
    {
        server->se_head = se;
        return se;
    }

    head_next = server->se_head->next;
    if (head_next)
        head_next->prev = se;
    server->se_head->next = se;
    se->next = head_next;
    se->prev = server->se_head;

    return se;
}

void 
server_del_event(server_t* server, server_event_t* se)
{
    server_event_t* next;
    server_event_t* prev;

    if (!server || !se)
    {
        warn("server_del_event(%p, %p): Something is NULL!\n",
                server, se);
        return;
    }

    server_ep_delfd(server, se->fd);
    if (se->close)
        se->close(server, se->data);
    else
        if (close(se->fd) == -1)
            error("close(%d): %s\n", se->fd, ERRSTR);

    prev = se->prev;
    next = se->next;
    if (prev)
        prev->next = next;
    if (next)
        next->prev = prev;
    if (se == server->se_head)
        server->se_head = next;

    free(se);
}

server_event_t* 
server_get_event(server_t* server, i32 fd)
{
    server_event_t* node;

    node = server->se_head;

    while (node)
    {
        if (node->fd == fd)
            return node;
        node = node->next;
    }

    return NULL;
}

upload_token_t* 
server_new_upload_token(server_t* server, u32 user_id)
{
    upload_token_t* ut;
    upload_token_t* head_next;

    ut = calloc(1, sizeof(upload_token_t));
    ut->user_id = user_id;
    getrandom(&ut->token, sizeof(u32), 0);

    if (server->upload_token_head == NULL)
    {
        server->upload_token_head = ut;
        return ut;
    }

    head_next = server->upload_token_head->next;
    server->upload_token_head->next = ut;
    ut->next = head_next;
    ut->prev = server->upload_token_head;
    if (head_next)
        head_next->prev = ut;

    return ut;
}

upload_token_t* 
server_get_upload_token(server_t* server, u32 token)
{
    upload_token_t* node;

    node = server->upload_token_head;

    while (node) 
    {
        if (node->token == token)
            return node;
        node = node->next;
    }

    return NULL;
}

void 
server_del_upload_token(server_t* server, upload_token_t* upload_token)
{
    upload_token_t* next;
    upload_token_t* prev;

    if (!server || !upload_token)
    {
        warn("del_upload_token(%p, %p): Something is NULL!\n", server, upload_token);
        return;
    }

    if (upload_token->timerfd)
    {
        server_ep_delfd(server, upload_token->timerfd);
        if (close(upload_token->timerfd) == -1)
            error("close(%d) ut timerfd: %s\n", upload_token->timerfd, ERRSTR);
    }

    next = upload_token->next;
    prev = upload_token->prev;
    if (next)
        next->prev = prev;
    if (prev)
        prev->next = next;
    if (upload_token == server->upload_token_head)
        server->upload_token_head = next;

    free(upload_token);
}