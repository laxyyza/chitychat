#include "server_ws_pld_hdlr.h"
#include "server_websocket.h"

bool json_bad(json_object* json, json_type type)
{
    if (json == NULL)
        return true;

    return !json_object_is_type(json, type);
}

enum client_recv_status 
server_ws_handle_text_frame(server_thread_t* th, client_t* client, 
                            char* buf, size_t buf_len) 
{
    json_object* type_json;
    json_object* respond_json;
    json_object* payload;
    json_tokener* tokener;
    const char* error_msg = NULL;
    const char* type;

    respond_json = json_object_new_object();
    tokener = json_tokener_new();
    payload = json_tokener_parse_ex(tokener, buf, 
                                            buf_len);
    json_tokener_free(tokener);

    if (!payload)
    {
        warn("WS JSON parse failed, message:\n%s\n", buf);
        // server_del_client(server, client);
        return RECV_DISCONNECT;
    }

    type_json = json_object_object_get(payload, "type");
    if (json_bad(type_json, json_type_string))
    {
        error_msg = "\"type\" is invalid or not found";
        goto send_error;
    }

    type = json_object_get_string(type_json);

    if (client->state & CLIENT_STATE_LOGGED_IN)
        error_msg = server_handle_logged_in_client(th, client, payload, 
                                                    respond_json, type);
    else
        error_msg = server_handle_not_logged_in_client(th, client, payload, 
                                                       respond_json, type);

send_error:
    if (error_msg)
    {
        verbose("Sending error: %s\n", error_msg);
        json_object_object_add(respond_json, "type", 
                                json_object_new_string("error"));
        json_object_object_add(respond_json, "from", payload);
        json_object_object_add(respond_json, "error_msg", 
                                    json_object_new_string(error_msg));

        ws_json_send(client, respond_json);
    }

    json_object_put(payload);
    json_object_put(respond_json);

    return RECV_OK;
}
