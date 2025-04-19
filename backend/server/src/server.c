#include "server.h"
#include "server_client.h"

i32
server_print_sockerr(i32 fd)
{
    i32 err;
    socklen_t len = sizeof(i32);
    if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &len) == -1)
        error("getsockopt(%d): %s\n", fd, ERRSTR);
    else if (err != ECONNRESET)
        error("socket fd:%d err:%d (%s)\n", fd, err, strerror(err));
    return err;
}

void
server_run(server_t* server)
{
    eworker_t* ew = server->main_ew;

    info("Server listening on IP: %s, port: %u, thread pool: %zu, using %s\n", 
         server->conf.addr_ip, server->conf.addr_port, server->tm.n_workers,
        (server->conf.disable_tls) ? "HTTP/WS" : "HTTPS/WSS");

    ew->server = server;
    ew->db.cmd = &server->db_commands;
    strncpy(ew->name, "ew:0", THREAD_NAME_LEN);
    tm_worker(ew);
}

static void 
server_del_all_clients(server_t* server)
{
    server_ght_t* ht = &server->client_ht;
    ht->ignore_resize = true;

    GHT_FOREACH(client_t* client, ht, {
        server_free_client(server->main_ew, client);
    });
    server_ght_destroy(ht);
    server_ght_destroy(&server->user_ht);
}

void
server_del_all_events(server_t* server)
{
    server_ght_t* ht = &server->event_ht;
    ht->ignore_resize = true;

    GHT_FOREACH(server_event_t* ev, ht, {
        server_del_event(server->main_ew, ev);
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
    server_db_free(server);
    server_close_magic(server);

    SSL_CTX_free(server->ssl_ctx);

    if (server->sigfd)
        close(server->sigfd);
    if (server->epfd)
        close(server->epfd);

    debug("Server stopped.\n");

    free(server);
}

static void
server_print_ssl_error(client_t* client, i32 ret, const char* from)
{
    i32 err = SSL_get_error(client->ssl, ret);
    if (err == SSL_ERROR_NONE || err == SSL_ERROR_ZERO_RETURN)
        return;
    error("SSL %s: %s (%d)\n", from, ERR_error_string(err, NULL), err);
}

ssize_t  
server_send(client_t* client, const void* buf, size_t len)
{
    ssize_t bytes_sent = -1;

    if (client->ssl)
    {
        pthread_mutex_lock(&client->ssl_mutex);
        if (client->err != CLIENT_ERR_SSL)
        {
            bytes_sent = SSL_write(client->ssl, buf, len);
            if (bytes_sent <= 0)
            {
                server_print_ssl_error(client, bytes_sent, "write");
                server_set_client_err(client, CLIENT_ERR_SSL);
            }
        }
        pthread_mutex_unlock(&client->ssl_mutex);
    }
    else 
    {
        bytes_sent = send(client->addr.sock, buf, len, 0);
        if (bytes_sent == -1)
            error("send (%s:%s): %s\n", client->addr.ip_str, client->addr.serv, ERRSTR);
    }

    return bytes_sent;
}

ssize_t 
server_recv(client_t* client, void* buf, size_t len)
{
    ssize_t bytes_recv = -1;

    if (client->ssl)
    {
        pthread_mutex_lock(&client->ssl_mutex);
        if (client->err != CLIENT_ERR_SSL)
        {
            bytes_recv = SSL_read(client->ssl, buf, len);
            if (bytes_recv <= 0)
            {
                server_print_ssl_error(client, bytes_recv, "read");
                server_set_client_err(client, CLIENT_ERR_SSL);
            }
        }
        pthread_mutex_unlock(&client->ssl_mutex);
    }
    else  
    {
        bytes_recv = recv(client->addr.sock, buf, len, 0);
        if (bytes_recv == -1)
            error("recv (%s:%s): %s\n", client->addr.ip_str, client->addr.serv, ERRSTR);
    }

    return bytes_recv;
}
