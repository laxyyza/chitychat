#include "server_events.h"
#include "chat/rtusm.h"
#include "server.h"
#include "server_client.h"
#include "server_tm.h"

static i32
server_ep_ctl(server_t* server, i32 op, i32 fd)
{
    i32 ret;

    struct epoll_event ev = {
        .data.fd = fd,
        .events = DEFAULT_EPEV
    };

    ret = epoll_ctl(server->epfd, op, fd, &ev);
    if (ret == -1)
        error("epoll_ctl op:%d, fd:%d\n", op, fd);

    return ret;
}

i32 
server_ep_rearm(server_t* server, i32 fd)
{
    return server_ep_ctl(server, EPOLL_CTL_MOD, fd);
}

i32 
server_ep_addfd(server_t* server, i32 fd)
{
    return server_ep_ctl(server, EPOLL_CTL_ADD, fd);
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

enum se_status
se_accept_conn(server_thread_t* th, UNUSED server_event_t* ev)
{
    i32 ret;
    server_t* server = th->server;
    client_t* client;

    client = calloc(1, sizeof(client_t));
    client->addr.len = server->addr_len;
    client->addr.version = server->conf.addr_version;
    client->addr.addr_ptr = (struct sockaddr*)&client->addr.ipv4;
    pthread_mutex_init(&client->ssl_mutex, NULL);
    client->addr.sock = accept(server->sock, client->addr.addr_ptr, &client->addr.len);
    if (client->addr.sock == -1)
    {
        error("accept: %s\n", ERRSTR);
        return SE_ERROR;
    }
    server_ght_insert(&th->server->client_ht, client->addr.sock, client);

    ret = server_client_ssl_handsake(server, client);
    if (ret == -1)
        goto error;
    server_get_client_info(client);

    //server_ep_addfd(server, client->addr.sock);
    if (server_new_event(server, client->addr.sock, client, se_read_client, se_close_client) == NULL)
        goto error;

    info("Client (fd:%d, IP: %s:%s) connected.\n", 
        client->addr.sock, client->addr.ip_str, client->addr.serv);

    return SE_OK;
error:
    server_free_client(th, client);
    return SE_ERROR;
}

enum se_status
se_read_client(server_thread_t* th, server_event_t* ev)
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
se_close_client(server_thread_t* th, server_event_t* ev)
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
    if (server_ep_addfd(server, fd) == -1)
    {
        error("ep_addfd %d failed\n", fd);
        goto err;
    }
    return se;
err:
    server_ep_delfd(server, fd);
    server_ght_del(&server->event_ht, fd);
    free(se);
    return NULL;
}

void 
server_del_event(server_thread_t* th, server_event_t* se)
{
    server_t* server = th->server;

    if (!server || !se)
    {
        warn("server_del_event(%p, %p): Something is NULL!\n",
                server, se);
        return;
    }

    server_ep_delfd(server, se->fd);
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
