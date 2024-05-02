#include "server_client.h"
#include "server.h"

client_t*   
server_get_client_fd(server_t* server, i32 fd)
{
    return server_ght_get(&server->client_ht, fd);
}

client_t*   
server_get_client_user_id(server_t* server, u64 id)
{
    server_ght_t* ht = &server->client_ht;
    // TODO: Use hash table to get client from user_id, not looping.
    server_ght_lock(ht);
    GHT_FOREACH(client_t* client, ht, {
        if (client->state & CLIENT_STATE_LOGGED_IN && client->dbuser->user_id == id)
        {
            server_ght_unlock(ht);
            return client;
        }
    });
    server_ght_unlock(ht);
    return NULL;
}

void 
server_free_client(server_thread_t* th, client_t* client)
{
    server_t* server = th->server;

    if (!client)
        return;

    info("Client (fd:%d, IP: %s:%s, host: %s) disconnected.\n", 
            client->addr.sock, client->addr.ip_str, client->addr.serv, client->addr.host);
    if (client->dbuser)
    {
        if (client->dbuser->user_id)
            debug("\tUser:%u %s '%s' logged out.\n", 
                client->dbuser->user_id, client->dbuser->username, client->dbuser->displayname);
    }

    if (client->ssl)
    {
        if (client->err == CLIENT_ERR_NONE)
            SSL_shutdown(client->ssl);
        SSL_free(client->ssl);
    }

    if (client->session && client->session->timerfd == 0 && th->server->running)
    {
        union timer_data data = {
            .session = client->session
        };
        // TODO: Make client session timer configurable
        server_timer_t* timer = server_addtimer(th, MINUTES(30), 
                                                TIMER_ONCE, TIMER_CLIENT_SESSION, 
                                                &data, sizeof(void*));
        if (timer)
            client->session->timerfd = timer->fd;
    }

    if (client->recv.data)
        free(client->recv.data);
    if (client->dbuser)
        free(client->dbuser);
    close(client->addr.sock);

    server_ght_del(&server->client_ht, client->addr.sock);

    free(client);
}

int 
server_client_ssl_handsake(server_t* server, client_t* client)
{
    i32 ret;
    client->ssl = SSL_new(server->ssl_ctx);
    if (!client->ssl)
    {
        error("SSL_new() failed.\n");
        return -1;
    }
    SSL_set_fd(client->ssl, client->addr.sock);
    ret = SSL_accept(client->ssl);
    if (ret == 1)
        return 0;
    server_set_client_err(client, CLIENT_ERR_SSL);
    return 0;
}

void 
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

void        
server_set_client_err(client_t* client, u16 err)
{
    client->err = err;
}
