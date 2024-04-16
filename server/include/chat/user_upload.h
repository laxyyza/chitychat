#ifndef _SERVER_CHAT_USER_UPLOAD_H_
#define _SERVER_CHAT_USER_UPLOAD_H_

#include "chat/upload_token.h"
#include "server_tm.h"
#include "server_client.h"

void server_handle_user_upload(server_thread_t* th, client_t* client, const http_t* http);

#endif // _SERVER_CHAT_USER_UPLOAD_H_
