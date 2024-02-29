#ifndef _SERVER_WEBSOCKET_H_
#define _SERVER_WEBSOCKET_H_

#include "common.h"
#include "server_client.h"

void server_ws_parse(server_t* server, client_t* client, u8* buf, size_t buf_len);

#endif // _SERVER_WEBSOCKET_H_