#include "server_events.h"
#include "chat/rtusm.h"
#include "server.h"
#include "server_client.h"
#include "server_eworker.h"
#include "chat/db_pipeline.h"
#include "server_tm.h"

i32
server_event_add(const server_t* server, server_event_t* se)
{
    i32 ret;

    if (se->listen_events == 0)
        se->listen_events = DEFAULT_EPEV;

    struct epoll_event ev = {
        .data.ptr = se,
        .events = se->listen_events
    };

    ret = epoll_ctl(server->epfd, EPOLL_CTL_ADD, se->fd, &ev);
    if (ret == -1)
        error("server_event_add on fd: %d\n", se->fd);
    return ret;
}

i32
server_event_remove(const server_t* server, const server_event_t* se)
{
    i32 ret;

    ret = epoll_ctl(server->epfd, EPOLL_CTL_DEL, se->fd, NULL);
    if (ret == -1)
        error("server_event_remove on fd: %d\n", se->fd);

    return ret;
}

i32 
server_event_rearm(const server_t* server, const server_event_t* se)
{
    i32 ret;

    struct epoll_event ev = {
        .data.ptr = (void*)se,
        .events = se->listen_events
    };

    ret = epoll_ctl(server->epfd, EPOLL_CTL_MOD, se->fd, &ev);
    if (ret == -1)
        error("server_event_rearm() on fd: %d\n", se->fd);

    return ret;
}

enum se_status
se_accept_conn(eworker_t* th, UNUSED server_event_t* ev)
{
    client_t* client;

    if ((client = server_accept_client(th)) == NULL)
        return SE_ERROR;

    info("Client (fd:%d, IP: %s:%s) connected.\n", 
        client->addr.sock, client->addr.ip_str, client->addr.serv);

    return SE_OK;
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
    else if (client->dbuser && th->server->running)
        server_rtusm_user_disconnect(th, client->dbuser);

    server_free_client(th, client);
    return SE_OK;
}

server_event_t* 
server_new_event(server_t* server, 
                 i32 fd, 
                 void* data, 
                 se_read_callback_t read_callback, 
                 se_close_callback_t close_callback)
{
    server_event_t* se;

    if (!server || !read_callback || (data && !close_callback))
    {
        warn("server_new_event(%p, %d, %p, %p, %p): Something is NULL!\n",
                server, fd, data, read_callback, close_callback);
        return NULL;
    }
    
    se = calloc(1, sizeof(server_event_t));
    se->fd = fd;
    se->data = data;
    se->read = read_callback;
    se->close = close_callback;
    se->listen_events = DEFAULT_EPEV;

    if (server_ght_insert(&server->event_ht, fd, se) == false)
    {
        error("new_event(): Failed to insert.\n");
        goto err;
    }
    if (server_event_add(server, se) == -1)
    {
        error("ep_addfd %d failed\n", fd);
        goto err;
    }
    return se;
err:
    server_event_remove(server, se);
    server_ght_del(&server->event_ht, fd);
    free(se);
    return NULL;
}

void 
server_del_event(eworker_t* th, server_event_t* se)
{
    server_t* server = th->server;

    if (!server || !se)
    {
        warn("server_del_event(%p, %p): Something is NULL!\n",
                server, se);
        return;
    }

    server_event_remove(server, se);
    if (se->close)
        se->close(th, se);
    else
        if (close(se->fd) == -1)
            error("del_event: close(%d): %s\n", se->fd, ERRSTR);

    if (server->running)
        server_ght_del(&server->event_ht, se->fd);

    free(se);
}

server_event_t* 
server_get_event(server_t* server, i32 fd)
{
    return server_ght_get(&server->event_ht, fd);
}

void 
server_process_event(eworker_t* ew, server_event_t* se)
{
    enum se_status ret;
    server_t* server = ew->server;
    const u32 ev = se->ep_events;
    const i32 fd = se->fd;

    if (ev & EPOLLERR)
    {
        se->err = server_print_sockerr(fd);
        server_del_event(ew, se);
    }
    else if (ev & (EPOLLRDHUP | EPOLLHUP))
    {
        verbose("fd: %d hang up.\n", fd);
        server_del_event(ew, se);
    }
    else if (ev & EPOLLIN)
    {
        ret = se->read(ew, se);
        if (ret == SE_CLOSE || ret == SE_ERROR)
            server_del_event(ew, se);
        else if (se->listen_events & EPOLLONESHOT)
            server_event_rearm(server, se);
    }
    else
        warn("Not handled fd: %d, ev: 0x%x\n", fd, ev);
}
