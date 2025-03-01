#include "server.h"
#include "chat/user_upload.h"

/*
 * Currently only the chat-backend will handle HTTP POST requests.
 */

enum client_recv_status
server_handle_http_post(eworker_t* th, client_t* client, const http_t* http)
{
    if (http_get_header(http, "Upload-Token") == NULL)
    {
        server_http_resp_error(client, 401, "Unauthorized");
        return RECV_DISCONNECT;
    }

    server_handle_user_upload(th, client, http);

    return RECV_OK;
}
