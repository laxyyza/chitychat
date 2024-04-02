#include "server_client.h"
#include "server.h"

client_t*   
server_new_client(server_t* server)
{
    client_t* client;
    
    client = calloc(1, sizeof(client_t));
    client->recv.overflow_check = CLIENT_OVERFLOW_CHECK_MAGIC;
    if (!server->client_head)
    {
        server->client_head = client;
        return client;
    }

    client->next = server->client_head->next;
    if (client->next)
        client->next->prev = client;
    server->client_head->next = client;
    client->prev = server->client_head;

    return client;
}

client_t*   
server_get_client_fd(server_t* server, i32 fd)
{
    client_t* node = server->client_head;

    while (node)
    {
        if (node->addr.sock == fd)
            return node;
        node = node->next;
    }

    return NULL;
}

client_t*   
server_get_client_user_id(server_t* server, u64 id)
{
    client_t* node = server->client_head;

    while (node)
    {
        if (node->state & CLIENT_STATE_LOGGED_IN && node->dbuser->user_id == id)
            return node;
        node = node->next;
    }

    return NULL;
}

void 
server_free_client(server_t* server, client_t* client)
{
    client_t* next;
    client_t* prev;

    if (!server || !client)
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
        SSL_shutdown(client->ssl);
        SSL_free(client->ssl);
    }

    if (client->session && client->session->timerfd == 0)
    {
        union timer_data data = {
            .session = client->session
        };
        // TODO: Make client session timer configurable
        server_timer_t* timer = server_addtimer(server, MINUTES(30), 
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

    prev = client->prev;
    next = client->next;
    if (prev)
        prev->next = next;
    if (next)
        next->prev = prev;

    if (client == server->client_head)
        server->client_head = next;
    free(client);
}

int 
server_client_ssl_handsake(server_t* server, client_t* client)
{
    i32 ret;
    client->secure = false;
    client->ssl = SSL_new(server->ssl_ctx);
    if (!client->ssl)
    {
        debug("client ssl is NULL!\n");
        return 0;
    }
    SSL_set_fd(client->ssl, client->addr.sock);
    ret = SSL_accept(client->ssl);
    if (ret <= 0) 
    {
        ret = SSL_get_error(client->ssl, ret);
        if (ERR_GET_LIB(ret) == ERR_LIB_SSL && ERR_GET_REASON(ret) == SSL_R_SSL_HANDSHAKE_FAILURE)
        {
            char buffer[1024];
            ERR_error_string_n(ret, buffer, 1024);
            debug("SSL/TLS error: %s\n", buffer);
            return -1;
        }
        else
            return 0;
    }

    client->secure = true;
    return 1;
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
