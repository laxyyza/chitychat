#include "server_ws_pld_hdlr.h"
#include "server_websocket.h"

enum client_recv_status 
server_ws_handle_text_frame(server_thread_t* th, client_t* client, 
                            char* buf, size_t buf_len) 
{
    char* buf_print = strndup(buf, buf_len);
    verbose("Web Socket message from fd:%d, IP: %s:%s:\n\t'%s'\n",
         client->addr.sock, client->addr.ip_str, client->addr.serv, buf_print);
    free(buf_print);

    json_object* respond_json = json_object_new_object();
    json_tokener* tokener = json_tokener_new();
    json_object* payload = json_tokener_parse_ex(tokener, buf, 
                                            buf_len);
    json_tokener_free(tokener);

    if (!payload)
    {
        warn("WS JSON parse failed, message:\n%s\n", buf);
        // server_del_client(server, client);
        return RECV_DISCONNECT;
    }

    json_object* type_json = json_object_object_get(payload, "type");
    const char* type = json_object_get_string(type_json);
    const char* error_msg = NULL;

    if (client->state & CLIENT_STATE_LOGGED_IN)
        error_msg = server_handle_logged_in_client(th, client, payload, 
                                                    respond_json, type);
    else
        error_msg = server_handle_not_logged_in_client(th, client, 
                                            payload, respond_json, type);

    if (error_msg)
    {
        verbose("Sending error: %s\n", error_msg);
        json_object_object_add(respond_json, "type", 
                                json_object_new_string("error"));
        json_object_object_add(respond_json, "error_msg", 
                                    json_object_new_string(error_msg));

        ws_json_send(client, respond_json);
    }

    json_object_put(payload);
    json_object_put(respond_json);

    return RECV_OK;
}
