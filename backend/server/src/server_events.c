#include "server_events.h"
#include "chat/rtusm.h"
#include "server.h"
#include "server_client.h"
#include "server_eworker.h"
#include "db/db_pipeline.h"
#include "server_tm.h"

static inline i32
do_epoll_add(eworker_t* ew, server_event_t* se)
{
    i32 ret;

    struct epoll_event ev = {
        .data.ptr = se,
        .events = se->listen_events
    };

    ret = epoll_ctl(ew->epfd, EPOLL_CTL_ADD, se->fd, &ev);

    if (ret == -1)
    {
        error("%s: EPFD=%d FD=%d epoll_ctl ADD: %s (%d)\n", 
              ew->name, ew->epfd, se->fd, ERRSTR, errno);
        return -1;
    }
    else
        atomic_fetch_add(&se->interests, 1);
    return ret;
}

static inline i32 
server_epoll_add_to_all(server_t* server, server_event_t* se)
{
    eworker_t* ew;

    for (i32 i = 0; i < server->tm.n_workers; i++)
    {
        ew = server->tm.workers + i;
        if (do_epoll_add(ew, se) == -1)
            return -1;
    }
    return 0;
}

static inline i32
eworker_epoll_add(eworker_t* ew, server_event_t* se)
{
    i32 ret;
    if (se->type == FD_SHARED)
    {
        ret = server_epoll_add_to_all(ew->server, se);
    }
    else
        ret = do_epoll_add(ew, se);

    return ret;
}

static inline i32 
do_epoll_del(const eworker_t* ew, server_event_t* se)
{
    i32 ret;
    ret = epoll_ctl(ew->epfd, EPOLL_CTL_DEL, se->fd, NULL);
    if (ret == -1)
        error("%s epoll_ctl DEL EPFD=%d FD=%d (%s): %s\n", 
              ew->name, ew->epfd, se->fd, se->debug_name, ERRSTR);
    return ret;
}

// static inline i32 
// server_epoll_remove_all(server_t* server, server_event_t* se)
// {
//     eworker_t* ew;
//
//     for (i32 i = 0; i < server->tm.n_workers; i++)
//     {
//         ew = server->tm.workers + i;
//         do_epoll_del(ew, se);
//     }
//
//     return 0;
// }

static inline i32
eworker_epoll_remove(const eworker_t* ew, server_event_t* se)
{
    i32 ret;

    // if (se->type == FD_SHARED)
    // {
    //     warn("%s deleting event fd:%d from all\n", ew->name, se->fd);
    //     ret = server_epoll_remove_all(ew->server, se);
    // }
    // else
    ret = do_epoll_del(ew, se);

    return ret;
}

i32 
eworker_epoll_rearm(const eworker_t* ew, server_event_t* se)
{
    i32 ret;

    se->listen_events = se->new_listen_events;

    struct epoll_event ev = {
        .data.ptr = (void*)se,
        .events = se->listen_events
    };

    ret = epoll_ctl(ew->epfd, EPOLL_CTL_MOD, se->fd, &ev);
    if (ret == -1)
    {
        error("%s: EPFD=%d FD=%d epoll_ctl MOD: %s\n", 
              ew->name, ew->epfd, se->fd, ERRSTR);
    }

    return ret;
}

i32 
server_epoll_rearm_all(const server_t* server, server_event_t* se)
{
    eworker_t* ew;
    for (i32 i = 0; i < server->tm.n_workers; i++)
    {
        ew = server->tm.workers + i;
        if (eworker_epoll_rearm(ew, se) == -1)
            return -1;
    }
    return 0;
}

enum se_status
se_accept_conn(eworker_t* th, server_event_t* ev)
{
    client_t* client;

    if ((client = server_accept_client(th, ev)) == NULL)
        return SE_ERROR;

    debug("TCP Connect: IP=[%s]\n", client->addr.ip_str);

    return SE_OK;
}

enum se_status
se_ssl_accept(UNUSED eworker_t* th, server_event_t* ev)
{
    client_t* client = ev->data;
    i32 ret;

    ret = SSL_accept(client->ssl);
    if (ret == 1)
    {
        verbose("%s SSL handshake completed.\n", client->addr.ip_str);
        ev->read = se_read_client;
        client->state &= ~CLIENT_STATE_SSL_HANDSHAKE;
        fcntl(client->addr.sock, F_SETFL, 0);
        ev->debug_name = "HTTP Client";
        return SE_OK;
    }
    else if (ret == 0)
        goto failed;
    else if (SSL_get_error(client->ssl, ret) == SSL_ERROR_WANT_READ)
        return SE_OK;

failed:
    debug("%s:%s SSL handshake failed. ssl_error: %d\n", client->addr.ip_str, client->addr.serv, SSL_get_error(client->ssl, ret));
    server_set_client_err(client, CLIENT_ERR_SSL);
    return SE_ERROR;
}

enum se_status
se_read_client(eworker_t* th, server_event_t* ev)
{
    ssize_t bytes_recv;
    u8* buf;
    size_t buf_size;
    size_t offset = 0;
    http_t* http;
    client_t* client;
    enum client_recv_status recv_status = RECV_OK;

    client = ev->data;
    http = client->recv.http;

    db_pipeline_set_ctx(&th->db, client);

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
    }

    bytes_recv = server_recv(client, buf + offset, buf_size - offset);
    if (bytes_recv <= 0)
        return SE_CLOSE;
    else if (http)
    {
        http->buf.total_recv += bytes_recv;
        verbose("HTTP recv: %zu/%zu\n", http->buf.total_recv, http->body_len);
        if ((size_t)bytes_recv >= buf_size)
        {
            server_handle_http(th, client, client->recv.http);
            client->recv.http = NULL;
        }
    }
    else
    {
        if (client->state & CLIENT_STATE_WEBSOCKET) 
            recv_status = server_ws_parse(th, client, buf, bytes_recv + offset); 
        else
            recv_status = server_http_parse(th, client, buf, bytes_recv);
    }

    if (recv_status != RECV_DISCONNECT && !client->recv.busy)
    {
        free(client->recv.data);
        client->recv.data = NULL;
        client->recv.data_size = 0;
        client->recv.offset = 0;
    }

    if (recv_status == RECV_DISCONNECT || recv_status == RECV_ERROR)
        return SE_CLOSE;

    return SE_OK;
}

enum se_status
se_close_client(eworker_t* th, server_event_t* ev)
{
    client_t* client = ev->data;

    if (ev->err == EPIPE)
        server_set_client_err(client, CLIENT_ERR_SSL);

    server_free_client(th, client);
    return SE_OK;
}

server_event_t* 
server_epoll_add_event(eworker_t* ew, add_event_args_t* args)
{
    server_event_t* se;
    i32 listen_events = (args->type == FD_SHARED) ? EPOLLEXCLUSIVE : 0;

    se = malloc(sizeof(server_event_t));
    se->fd = args->fd;
    se->data = args->data;
    se->debug_name = args->name;
    if (args->read_cb)
    {
        listen_events |= EPOLLIN;
        se->read = args->read_cb;
    }
    else
        se->read = NULL;

    if (args->write_cb)
    {
        listen_events |= EPOLLOUT;
        se->write = args->write_cb;
    }
    else
        se->write = NULL;

    if (args->close_cb)
    {
        listen_events |= EPOLLHUP;
        se->close = args->close_cb;
    }
    else
        se->close = NULL;

    se->new_listen_events = se->listen_events = listen_events;
    se->flags = 0;
    se->type = args->type;
    se->err = 0;
    atomic_init(&se->interests, 0);

    if (eworker_epoll_add(ew, se) == -1)
    {
        error("server_epoll_add failed for fd: %d\n", args->fd);
        free(se);
        return NULL;
    }
    return se;
}

void 
server_do_free_event(eworker_t* ew, server_event_t* se)
{
    if (se->close)
        se->close(ew, se);
    else if ((se->flags & SE_DONT_CLOSE_FD) == 0)
    {
        if (close(se->fd) == -1)
            error("del_event: close(%d): %s\n", se->fd, ERRSTR);
    }

    free(se);
}

void 
eworker_del_event(eworker_t* ew, server_event_t* se)
{
    if (!ew || !se)
        return;

    eworker_epoll_remove(ew, se);

    if (atomic_fetch_sub(&se->interests, 1) == 1)
        server_do_free_event(ew, se);
}

void 
server_process_event(eworker_t* ew, server_event_t* se)
{
    enum se_status ret;
    // server_t* server = ew->server;
    const u32 ev = se->ep_events;
    const i32 fd = se->fd;

    if (ev & EPOLLERR)
    {
        se->err = server_print_sockerr(fd);
        eworker_del_event(ew, se);
        return;
    }
    else if (ev & (EPOLLRDHUP | EPOLLHUP))
    {
        eworker_del_event(ew, se);
        return;
    }

    if (ev & EPOLLIN)
    {
        ret = se->read(ew, se);
        if (ret == SE_CLOSE || ret == SE_ERROR)
        {
            eworker_del_event(ew, se);
            return;
        }
    }

    if (ev & EPOLLOUT)
    {
        ret = se->write(ew, se);
        if (ret == SE_CLOSE || ret == SE_ERROR)
        {
            eworker_del_event(ew, se);
            return;
        }
    }

    if (se->new_listen_events != se->listen_events)
        eworker_epoll_rearm(ew, se);
}

void 
server_make_event_shared(eworker_t* ew, server_event_t* se)
{
    if (se->type == FD_SHARED)
        return;
    // if (server_ght_del_opt(&ew->event_ht, se->fd, true) == false)
    //     return;

    se->type = FD_SHARED;

    for (i32 i = 0; i < ew->server->tm.n_workers; i++)
    {
        eworker_t* worker = ew->server->tm.workers + i;
        if (worker != ew)
            eworker_epoll_add(worker, se);
    }
}
