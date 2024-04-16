#include "chat/ws_text_frame.h"
#include "chat/user_login.h"
#include "chat/user_req.h"

bool json_bad(json_object* json, json_type type)
{
    if (json == NULL)
        return true;

    return !json_object_is_type(json, type);
}

enum client_recv_status 
server_ws_handle_text_frame(server_thread_t* th, 
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

    cmd_json = json_object_object_get(payload, "cmd");
    if (json_bad(cmd_json, json_type_string))
    {
        error_msg = "\"cmd\" is invalid or not found";
        goto send_error;
    }

    cmd = json_object_get_string(cmd_json);

    if (client->state & CLIENT_STATE_LOGGED_IN)
        error_msg = server_handle_logged_in_client(th, client, payload, 
                                                    respond_json, cmd);
    else
        error_msg = server_handle_not_logged_in_client(th, client, payload, 
                                                       respond_json, cmd);

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
