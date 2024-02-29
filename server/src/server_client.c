#include "server_client.h"
#include "server.h"

client_t*   server_new_client(server_t* server)
{
    if (server->client_head && server->client_head->addr.sock == 0)
        return server->client_head;

    client_t* client = calloc(1, sizeof(client_t));
    if (!server->client_head)
        return client;

    client->next = server->client_head->next;
    if (client->next)
        client->next->prev = client;
    server->client_head->next = client;
    client->prev = server->client_head;

    debug("New client node: Prev: %p\tNext: %p\n", client->prev, client->next);

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
        if (node->state & CLIENT_STATE_WEBSOCKET && node->dbuser.user_id == id)
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
    close(client->addr.sock);

    client_t* prev = client->prev;
    client_t* next = client->next;
    if (prev)
        prev->next = next;
    if (next)
        next->prev = prev;

    if (client == server->client_head)
        memset(client, 0, sizeof(client_t));
    else
        free(client);
}