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
    return server_ght_get(&server->user_ht, id);
}

client_t*
server_accept_client(eworker_t* th, server_event_t* ev)
{
    client_t* client;
    server_t* server = th->server;

    client = calloc(1, sizeof(client_t));
    client->addr.len = server->addr_len;
    client->addr.version = server->conf.addr_version;
    client->addr.addr_ptr = (struct sockaddr*)&client->addr.ipv4;
    client->addr.sock = accept4(ev->fd, client->addr.addr_ptr, &client->addr.len, (server->conf.disable_tls) ? 0 : SOCK_NONBLOCK);
    if (client->addr.sock == -1)
    {
        error("accept: %s\n", ERRSTR);
        goto err;
    }

    server_get_client_info(client);
    server_ght_insert(&server->client_ht, client->addr.sock, client);

    if (server->conf.disable_tls == false)
    {
        client->ssl = SSL_new(server->ssl_ctx);
        SSL_set_fd(client->ssl, client->addr.sock);
        pthread_mutex_init(&client->ssl_mutex, NULL);
    }

    if (server_new_event(server, client->addr.sock, client, 
                         (server->conf.disable_tls) ? se_read_client : se_ssl_accept, se_close_client) == NULL)
        goto err;

    return client;
err:
    server_free_client(th, client);
    return NULL;
}

void 
server_free_client(eworker_t* ew, client_t* client)
{
    server_t* server = ew->server;

    if (!client)
        return;
    server_ght_del(&server->client_ht, client->addr.sock);
    if (client->state & CLIENT_STATE_SESSION_PENDING)
        server_ght_del(&ew->server->client_by_tmptoken_ht, client->tmptoken);

    debug("Client (IP: %s:%s) closed.\n", 
            client->addr.ip_str, client->addr.serv);
    if (client->dbuser)
    {
        if (client->dbuser->user_id)
            info("\tUser:%u %s '%s' logged out.\n", 
                client->dbuser->user_id, client->dbuser->username, client->dbuser->displayname);
    }

    if (client->ssl)
    {
        if (client->err == CLIENT_ERR_NONE)
            SSL_shutdown(client->ssl);
        SSL_free(client->ssl);
        pthread_mutex_destroy(&client->ssl_mutex);
    }

    if (client->session_uuid[0])
    {
        server_ght_del(&ew->server->client_by_session_ht, server_ght_hash_uuid(client->session_uuid));
    }

    if (client->recv.data)
        free(client->recv.data);
    if (client->dbuser)
    {
        for (u32 i = 0; i < client->dbuser->connected_clients.count; i++)
        {
            client_t* other_client = *(client_t**)array_idx(&client->dbuser->connected_clients, i);
            if (other_client == client)
            {
                array_erase(&client->dbuser->connected_clients, i);
                break;
            }
        }

        if (ew->server->running)
            server_rtusm_user_disconnect(ew, client->dbuser);

        if (client->dbuser->connected_clients.count == 0 && client->dbuser->msg_tokens.tokens > 0)
        {
            natsSubscription_Unsubscribe(client->dbuser->sub);
            server_ght_del(&ew->server->user_ht, client->dbuser->user_id);
        }
    }
    close(client->addr.sock);

    free(client);
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
