#include "chat/upload_token.h"
#include "server.h"

upload_token_t* 
server_new_upload_token(eworker_t* ew, u32 user_id)
{
    upload_token_t* ut;
    server_timer_t* timer;
    union timer_data timer_data;
    server_t* server = ew->server;

    ut = calloc(1, sizeof(upload_token_t));
    ut->user_id = user_id;
    ut->type = UT_USER_PFP;
    getrandom(&ut->token, sizeof(u32), 0);

    server_ght_insert(&server->upload_token_ht, ut->token, ut);

    timer_data.ut = ut;
    // TODO: Make seconds configurable
    timer = server_addtimer(ew, 10, 
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
server_new_upload_token_attach(eworker_t* ew)
{
    upload_token_t* ut;

    ut = server_new_upload_token(ew, 0);
    ut->type = UT_MSG_ATTACHMENT;

    return ut;
}

upload_token_t* 
server_get_upload_token(server_t* server, u32 token)
{
    return server_ght_get(&server->upload_token_ht, token);
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
server_del_upload_token(eworker_t* ew, upload_token_t* upload_token)
{
    server_t* server = ew->server;
    server_event_t* se_timer;
    dbmsg_t* msg;

    if (!server || !upload_token)
    {
        warn("del_upload_token(%p, %p): Someewing is NULL!\n", server, upload_token);
        return;
    }

    if (upload_token->timerfd)
    {
        se_timer = server_get_event(server, upload_token->timerfd);
        server_del_event(ew, se_timer);
    }

    if (upload_token->type == UT_MSG_ATTACHMENT)
    {
        msg = &upload_token->msg_state.msg;
        if (msg->attachments && msg->attachments_inheap)
            free(upload_token->msg_state.msg.attachments);
        if (msg->attachments_json)
            json_object_put(msg->attachments_json);
    }

    server_ght_del(&server->upload_token_ht, upload_token->token);

    free(upload_token);
}
