#include "server.h"
#include "chat/user_upload.h"

/*
 * Currently only the chat-backend will handle HTTP POST requests.
 */

void 
server_handle_http_post(server_thread_t* th, client_t* client, const http_t* http)
{
    server_handle_user_upload(th, client, http);
}
