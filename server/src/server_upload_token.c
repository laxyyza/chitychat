#include "server_upload_token.h"
#include "server.h"

upload_token_t* 
server_new_upload_token(server_t* server, u32 user_id)
{
    upload_token_t* ut;
    upload_token_t* head_next;

    ut = calloc(1, sizeof(upload_token_t));
    ut->user_id = user_id;
    getrandom(&ut->token, sizeof(u32), 0);

    if (server->upload_token_head == NULL)
    {
        server->upload_token_head = ut;
        return ut;
    }

    head_next = server->upload_token_head->next;
    server->upload_token_head->next = ut;
    ut->next = head_next;
    ut->prev = server->upload_token_head;
    if (head_next)
        head_next->prev = ut;

    return ut;
}

upload_token_t* 
server_get_upload_token(server_t* server, u32 token)
{
    upload_token_t* node;

    node = server->upload_token_head;

    while (node) 
    {
        if (node->token == token)
            return node;
        node = node->next;
    }

    return NULL;
}

void 
server_del_upload_token(server_t* server, upload_token_t* upload_token)
{
    upload_token_t* next;
    upload_token_t* prev;

    if (!server || !upload_token)
    {
        warn("del_upload_token(%p, %p): Something is NULL!\n", server, upload_token);
        return;
    }

    if (upload_token->timerfd)
    {
        server_ep_delfd(server, upload_token->timerfd);
        if (close(upload_token->timerfd) == -1)
            error("close(%d) ut timerfd: %s\n", upload_token->timerfd, ERRSTR);
    }

    next = upload_token->next;
    prev = upload_token->prev;
    if (next)
        next->prev = prev;
    if (prev)
        prev->next = next;
    if (upload_token == server->upload_token_head)
        server->upload_token_head = next;

    free(upload_token);
}