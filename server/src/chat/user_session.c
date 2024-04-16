#include "chat/user_session.h"
#include "server.h"

session_t* 
server_new_user_session(server_t* server, client_t* client)
{
    session_t* new_sesion;
    session_t* head_next;

    if (!server || !client || !client->dbuser)
    {
        warn("new_user_session(%p, %p) dbuser: %p: Something is NULL!\n", server, client, client->dbuser);
        return NULL;
    }

    new_sesion = calloc(1, sizeof(session_t));
    getrandom(&new_sesion->session_id, sizeof(u32), 0);
    new_sesion->user_id = client->dbuser->user_id;

    client->session = new_sesion;

    if (server->session_head == NULL)
    {
        server->session_head = new_sesion;
        return new_sesion;
    }

    head_next = server->session_head->next;
    server->session_head->next = new_sesion;
    new_sesion->next = head_next;
    if (head_next)
        head_next->prev = new_sesion;
    new_sesion->prev = server->session_head;

    return new_sesion;
}

session_t* 
server_get_user_session(server_t* server, u32 session_id)
{
    session_t* node;

    node = server->session_head;

    while (node)
    {
        if (node->session_id == session_id)
            return node;
        node = node->next;
    }

    return NULL;
}

session_t* 
server_get_client_session_uid(server_t* server, u32 user_id)
{
    session_t* node;

    node = server->session_head;

    while (node)
    {
        if (node->user_id == user_id)
            return node;
        node = node->next;
    }

    return NULL;
}

void 
server_del_user_session(server_t* server, session_t* session)
{
    session_t* next;
    session_t* prev;

    if (!server || !session)
    {
        warn("del_user_session(%p, %p): Something is NULL!\n", server, session);
        return;
    }

    verbose("Deleting session: %u for user %u\n", session->session_id, session->user_id);

    next = session->next;
    prev = session->prev;

    if (next)
        next->prev = prev;
    if (prev)
        prev->next = next;
    if (session == server->session_head)
        server->session_head = next;

    free(session);
}
