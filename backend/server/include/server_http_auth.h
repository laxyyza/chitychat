#ifndef _SERVER_HTTP_AUTH_H_
#define _SERVER_HTTP_AUTH_H_

#include "server_eworker.h"
#include "server_client.h"

enum client_recv_status server_handle_auth(eworker_t* ew, client_t* client, http_t* http);

#endif // _SERVER_HTTP_AUTH_H_
