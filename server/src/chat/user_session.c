#include "chat/user_session.h"
#include "server.h"

session_t* 
server_new_user_session(server_t* server, client_t* client)
{
    session_t* new_sesion;

    if (!server || !client)
    {
        warn("new_user_session(%p, %p) dbuser: %p: Something is NULL!\n", server, client);
        return NULL;
    }

    new_sesion = calloc(1, sizeof(session_t));
    getrandom(&new_sesion->session_id, sizeof(u32), 0);

    client->session = new_sesion;

    server_ght_insert(&server->session_ht, new_sesion->session_id, new_sesion);

    return new_sesion;
}

session_t* 
server_get_user_session(server_t* server, u32 session_id)
{
    return server_ght_get(&server->session_ht, session_id);
}

session_t* 
server_get_client_session_uid(server_t* server, u32 user_id)
{
    server_ght_t* ht = &server->session_ht;

    server_ght_lock(ht);
    GHT_FOREACH(session_t* session, ht, {
        if (session->user_id == user_id)
        {
            server_ght_unlock(ht);
            return session;
        }
    });
    server_ght_unlock(ht);
    return NULL;
}

void 
server_del_user_session(server_t* server, session_t* session)
{
    if (!server || !session)
    {
        warn("del_user_session(%p, %p): Something is NULL!\n", server, session);
        return;
    }

    verbose("Deleting session: %u for user %u\n", session->session_id, session->user_id);

    server_ght_del(&server->session_ht, session->session_id);

    free(session);
}
