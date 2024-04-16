#include "chat/upload_token.h"
#include "server.h"

upload_token_t* 
server_new_upload_token(server_t* server, u32 user_id)
{
    upload_token_t* ut;
    upload_token_t* head_next;
    server_timer_t* timer;
    union timer_data timer_data;

    ut = calloc(1, sizeof(upload_token_t));
    ut->user_id = user_id;
    ut->type = UT_USER_PFP;
    getrandom(&ut->token, sizeof(u32), 0);

    if (server->upload_token_head == NULL)
    {
        server->upload_token_head = ut;
        goto add_timer;
    }

    head_next = server->upload_token_head->next;
    server->upload_token_head->next = ut;
    ut->next = head_next;
    ut->prev = server->upload_token_head;
    if (head_next)
        head_next->prev = ut;

add_timer:
    timer_data.ut = ut;
    // TODO: Make seconds configurable
    timer = server_addtimer(server, 10, 
                            TIMER_ONCE, TIMER_UPLOAD_TOKEN, 
                            &timer_data, sizeof(void*));
    if (timer)
    {
        ut->timerfd = timer->fd;
        ut->timer_seconds = timer->seconds;
    }

    return ut;
}

upload_token_t* 
server_new_upload_token_attach(server_t* server, dbmsg_t* msg)
{
    upload_token_t* ut;

    ut = server_new_upload_token(server, 0);
    ut->type = UT_MSG_ATTACHMENT;
    memcpy(&ut->msg_state.msg, msg, sizeof(dbmsg_t));

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

ssize_t         
server_send_upload_token(client_t* client, const char* packet_type, upload_token_t* ut)
{
    json_object* respond_json;
    ssize_t bytes_sent;

    respond_json = json_object_new_object();

    json_object_object_add(respond_json, "cmd", 
            json_object_new_string(packet_type));
    json_object_object_add(respond_json, "upload_token", 
            json_object_new_uint64(ut->token));

    bytes_sent = ws_json_send(client, respond_json);

    json_object_put(respond_json);

    return bytes_sent;
}

void 
server_del_upload_token(server_t* server, upload_token_t* upload_token)
{
    upload_token_t* next;
    upload_token_t* prev;
    server_event_t* se_timer;
    dbmsg_t* msg;

    if (!server || !upload_token)
    {
        warn("del_upload_token(%p, %p): Something is NULL!\n", server, upload_token);
        return;
    }

    if (upload_token->timerfd)
    {
        se_timer = server_get_event(server, upload_token->timerfd);
        server_del_event(server, se_timer);
    }

    if (upload_token->type == UT_MSG_ATTACHMENT)
    {
        msg = &upload_token->msg_state.msg;
        if (msg->attachments && msg->attachments_inheap)
            free(upload_token->msg_state.msg.attachments);
        if (msg->attachments_json)
            json_object_put(msg->attachments_json);
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
