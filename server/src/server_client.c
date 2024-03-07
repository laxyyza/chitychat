#include "server_client.h"
#include "server.h"

client_t*   server_new_client(server_t* server)
{
    client_t* client = calloc(1, sizeof(client_t));
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

client_t*   server_get_client_fd(server_t* server, i32 fd)
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

client_t*   server_get_client_user_id(server_t* server, u64 id)
{
    client_t* node = server->client_head;

    while (node)
    {
        if (node->state & CLIENT_STATE_LOGGED_IN && node->dbuser.user_id == id)
            return node;
        node = node->next;
    }

    return NULL;
}

void server_del_client(server_t* server, client_t* client)
{
    if (!server || !client)
        return;

    info("Client (fd:%d, ip: %s:%s, host: %s) disconnected.\n", 
        client->addr.sock, client->addr.ip_str, client->addr.serv, client->addr.host);

    server_ep_delfd(server, client->addr.sock);
    if (client->ssl)
    {
        SSL_shutdown(client->ssl);
        SSL_free(client->ssl);
    }
    close(client->addr.sock);

    client_t* prev = client->prev;
    client_t* next = client->next;
    if (prev)
        prev->next = next;
    if (next)
        next->prev = prev;

    if (client == server->client_head)
        server->client_head = next;
    free(client);
}

bool server_client_ssl_handsake(server_t* server, client_t* client)
{
    client->ssl = SSL_new(server->ssl_ctx);
    if (!client->ssl)
        debug("client ssl is NULL!\n");
    SSL_set_fd(client->ssl, client->addr.sock);
    i32 ret = SSL_accept(client->ssl);
    if (ret <= 0) 
    {
        ERR_print_errors_fp(stderr);
        client->secure = false;
        return false;
    }
    else
    {
        client->secure = true;
        return true;
    }
}