#include "chat/ws_text_frame.h"
#include "chat/user_login.h"
#include "chat/cmd.h"
#include "nano_timer.h"

bool 
json_bad(json_object* json, json_type type)
{
    if (json == NULL)
        return true;

    return !json_object_is_type(json, type);
}

enum client_recv_status 
server_ws_handle_text_frame(eworker_t* ew, 
                            client_t* client, 
                            char* buf, 
                            size_t buf_len) 
{
    json_object* cmd_json;
    json_object* respond_json;
    json_object* payload;
    json_tokener* tokener;
    const char* error_msg = NULL;
    const char* cmd;

    respond_json = json_object_new_object();
    tokener = json_tokener_new();
    payload = json_tokener_parse_ex(tokener, buf, buf_len + 1);
    json_tokener_free(tokener);

    if (!payload)
    {
        warn("WS JSON parse failed, message:\n%s\n", buf);
        return RECV_DISCONNECT;
    }

    if (client->dbuser)
    {
        token_bucket_t* bucket = &client->dbuser->msg_tokens;
        hr_time_t current_time;
        nano_gettime(&current_time);
        f64 current_time_s = nano_time_s(&current_time);
        f64 time_elapsed = current_time_s - bucket->last_token_refil;
        i32 new_tokens = (i32)(time_elapsed * TOKEN_REFIL_RATE);

        bucket->tokens--;

        if (new_tokens > 0)
        {
            bucket->last_token_refil = current_time_s;
            bucket->tokens += new_tokens;
            if (bucket->tokens > TOKEN_CAP)
                bucket->tokens = TOKEN_CAP;
        }

        if (bucket->tokens <= 0)
        {
            if (bucket->tokens <= RATE_LIMIT_DISCONNECT)
                return RECV_DISCONNECT;

            time_elapsed = current_time_s - bucket->last_respond_time;
            if (time_elapsed >= RATE_LIMIT_RESPOND_RATE)
            {
                error_msg = "Rate limited";
                bucket->last_respond_time = current_time_s;
            }
            goto send_error;
        }
    }

    cmd_json = json_object_object_get(payload, "cmd");
    if (json_bad(cmd_json, json_type_string))
    {
        error_msg = "\"cmd\" is invalid or not found";
        goto send_error;
    }

    cmd = json_object_get_string(cmd_json);

    error_msg = server_exec_chatcmd(cmd, ew, client, payload, respond_json);
send_error:
    if (error_msg)
    {
        verbose("Sending error: %s\n", error_msg);
        json_object_object_add(respond_json, "cmd", 
                                json_object_new_string("error"));
        json_object_object_add(respond_json, "from", 
                               json_object_get(payload));
        json_object_object_add(respond_json, "error_msg", 
                                json_object_new_string(error_msg));

        ws_json_send(client, respond_json);
    }

    json_object_put(payload);
    json_object_put(respond_json);

    return RECV_OK;
}
